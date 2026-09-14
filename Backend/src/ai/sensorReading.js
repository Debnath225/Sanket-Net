import { createEmbedding } from "./rag.js";

const number = (value) => Number.isFinite(Number(value)) ? Number(value) : null;
const integer = (value) => Number.isInteger(Number(value)) ? Number(value) : null;
const date = (value, fallback) => { const result = new Date(value || fallback); return Number.isNaN(result.getTime()) ? new Date(fallback) : result; };

export function sensorDocument(record) {
  const prediction = record.prediction || {};
  return [
    `Sensor reading from node ${record.node_id || record.source_node || "UNKNOWN"}.`,
    `Captured at ${record.capturedAt || record.receivedAt || new Date().toISOString()}.`,
    `Temperature ${record.temperature ?? "unknown"} C; humidity ${record.humidity ?? "unknown"} percent; rainfall ${record.rainfall_mm ?? record.rainfallMM ?? "unknown"} mm; water level ${record.water_level ?? record.waterLevel ?? "unknown"}; soil moisture ${record.soil_moisture ?? record.soilMoisture ?? "unknown"} percent.`,
    `Predicted hazard ${prediction.hazard || record.hazard || "unknown"}; risk ${prediction.risk ?? record.risk_score ?? "unknown"} percent.`,
  ].join(" ");
}

export function toSensorReading(record) {
  const document = sensorDocument(record);
  const prediction = record.prediction || null;
  return {
    nodeId: String(record.node_id || record.source_node || "UNKNOWN"), topic: record.topic ? String(record.topic) : null, sequence: integer(record.sequence ?? record.seq),
    capturedAt: date(record.capturedAt ?? record.timestamp ?? record.at, record.receivedAt || Date.now()), receivedAt: date(record.receivedAt, Date.now()),
    latitude: number(record.latitude), longitude: number(record.longitude), batteryMv: integer(record.battery_mv ?? record.batteryMv), temperature: number(record.temperature), humidity: number(record.humidity), pressure: number(record.pressure), rainfallMm: number(record.rainfall_mm ?? record.rainfallMM), waterLevel: number(record.water_level ?? record.waterLevel), soilMoisture: number(record.soil_moisture ?? record.soilMoisture), windSpeed: number(record.wind_speed ?? record.windSpeed), vibration: number(record.vibration), mq2: number(record.mq2), mq135: number(record.mq135), turbidity: number(record.turbidity),
    hazard: prediction?.hazard || record.hazard || null, riskScore: number(prediction?.risk ?? record.risk_score), prediction, rawPayload: record, document, embedding: createEmbedding(document),
  };
}
