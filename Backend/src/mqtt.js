import fs from "node:fs";
import mqtt from "mqtt";
import { predict } from "./ai/predictor.js";

function addRecord(store, kind, value) {
  void store.add(kind, value).catch((error) => {
    console.error(`[store] Failed to persist ${kind}: ${error.message}`);
  });
}
export function startMqtt(config, store) {
  if (!config.mqttUrl) {
    addRecord(store, "events", {
      level: "warning",
      message: "MQTT_URL is not configured",
      at: new Date().toISOString(),
    });
    return null;
  }

  const options = {
    clientId: config.mqttClientId,
    username: config.mqttUsername || undefined,
    password: config.mqttPassword || undefined,
    reconnectPeriod: 5000,
    clean: true,
    ...(config.mqttCaFile ? { ca: fs.readFileSync(config.mqttCaFile) } : {}),
  };
  const client = mqtt.connect(config.mqttUrl, options);
  const topic = `${config.mqttTopicRoot}/+/+`;

  client.on("connect", () => {
    client.subscribe(topic, { qos: 1 });
    addRecord(store, "events", {
      level: "info",
      message: `MQTT connected; subscribed to ${topic}`,
      at: new Date().toISOString(),
    });
  });
  client.on("reconnect", () =>
    addRecord(store, "events", {
      level: "info",
      message: "MQTT reconnecting",
      at: new Date().toISOString(),
    }),
  );
  client.on("error", (error) =>
    addRecord(store, "events", {
      level: "error",
      message: `MQTT error: ${error.message}`,
      at: new Date().toISOString(),
    }),
  );
  client.on("message", (topicName, buffer) => {
    try {
      const payload = JSON.parse(buffer.toString("utf8"));
      ingest(topicName, payload, store);
    } catch (error) {
      addRecord(store, "events", {
        level: "error",
        message: `Invalid MQTT JSON on ${topicName}: ${error.message}`,
        at: new Date().toISOString(),
      });
    }
  });
  return client;
}

function ingest(topic, payload, store) {
  const parts = topic.split("/");
  const type = parts[1];
  const nodeId =
    payload.node_id ?? payload.source_node ?? parts[2] ?? "unknown";
  const record = {
    ...payload,
    node_id: nodeId,
    topic,
    receivedAt: new Date().toISOString(),
  };

  if (type === "telemetry") {
    record.prediction = predict(record);
    addRecord(store, "telemetry", record);
    if (record.prediction.risk >= 75)
      addRecord(store, "alerts", {
        ...record.prediction,
        node_id: nodeId,
        source: "ai",
        createdAt: record.receivedAt,
      });
  } else if (type === "alerts") {
    addRecord(store, "alerts", record);
  } else if (type === "heartbeat") {
    addRecord(store, "heartbeats", record);
  } else {
    addRecord(store, "events", {
      level: "info",
      message: `Received ${type} from ${nodeId}`,
      data: record,
      at: record.receivedAt,
    });
  }
}
