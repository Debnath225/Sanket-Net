import { Router } from "express";
import { predict } from "../ai/predictor.js";
import { createAnswerGenerator } from "../ai/llm.js";
import { createDashboardController } from "../controllers/dashboardController.js";
import { createChatController } from "../controllers/chatController.js";
import { createAdminController } from "../controllers/adminController.js";

export function createDashboardRouter(store, config) {
  const router = Router();
  const dashboard = createDashboardController(store);
  const chat = createChatController(store, createAnswerGenerator(config));
  const admin = createAdminController(store, config);

  router.get("/snapshot", dashboard.snapshot);
  router.get("/telemetry", dashboard.telemetry);
  router.get("/sensor-readings", dashboard.sensorReadings);
  router.get("/alerts", dashboard.alerts);
  router.get("/events", dashboard.events);
  router.get("/nodes", dashboard.listNodes);
  router.get("/nodes/:nodeId", dashboard.node);
  router.get("/network/summary", dashboard.networkSummary);
  router.post("/chat", chat.ask);
  router.get("/conversations", chat.list);
  router.get("/conversations/:id", chat.messages);
  router.delete("/conversations/:id", chat.remove);
  router.get("/admin/status", admin.requireAdmin, admin.status);
  router.get("/admin/users", admin.requireAdmin, admin.users);
  router.patch("/admin/users/:username", admin.requireAdmin, admin.updateUser);
  router.get("/admin/conversations", admin.requireAdmin, admin.conversations);
  router.delete("/admin/conversations/:id", admin.requireAdmin, admin.deleteConversation);
  router.post("/predict", (request, response) => response.json(predict(request.body || {})));
  router.post("/ingest", async (request, response, next) => {
    const payload = request.body || {};
    const topic = request.headers["x-mqtt-topic"] || `${config.mqttTopicRoot}/telemetry/${payload.node_id || "manual"}`;
    try { await store.add("telemetry", { ...payload, topic, receivedAt: new Date().toISOString(), prediction: predict(payload) }); response.status(202).json({ accepted: true, persistence: store.persistenceMode }); } catch (error) { next(error); }
  });
  return router;
}
