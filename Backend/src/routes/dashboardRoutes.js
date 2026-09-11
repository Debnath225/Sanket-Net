import { Router } from "express";
import { predict } from "../ai/predictor.js";
import { createDashboardController } from "../controllers/dashboardController.js";

export function createDashboardRouter(store, config) {
  const router = Router();
  const dashboard = createDashboardController(store);

  router.get("/snapshot", dashboard.snapshot);
  router.get("/telemetry", dashboard.telemetry);
  router.get("/alerts", dashboard.alerts);
  router.get("/events", dashboard.events);
  router.get("/nodes", dashboard.listNodes);
  router.get("/nodes/:nodeId", dashboard.node);
  router.get("/network/summary", dashboard.networkSummary);
  router.post("/predict", (request, response) =>
    response.json(predict(request.body || {})),
  );
  router.post("/ingest", async (request, response, next) => {
    const payload = request.body || {};
    const topic =
      request.headers["x-mqtt-topic"] ||
      `${config.mqttTopicRoot}/telemetry/${payload.node_id || "manual"}`;
    try {
      await store.add("telemetry", {
        ...payload,
        topic,
        receivedAt: new Date().toISOString(),
        prediction: predict(payload),
      });
      response.status(202).json({ accepted: true, persistence: store.persistenceMode });
    } catch (error) {
      next(error);
    }
  });
  return router;
}
