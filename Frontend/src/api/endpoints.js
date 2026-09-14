import { request } from "./client.js";

export const endpoints = {
  // Operational & Public Telemetry
  getSnapshot: (token) => request("/api/snapshot", { token }),
  getTelemetry: (limit = 100, token) => request(`/api/telemetry?limit=${limit}`, { token }),
  getSensorReadings: (limit = 100, nodeId = "", token) =>
    request(`/api/sensor-readings?limit=${limit}${nodeId ? `&nodeId=${encodeURIComponent(nodeId)}` : ""}`, { token }),
  getAlerts: (limit = 100, token) => request(`/api/alerts?limit=${limit}`, { token }),
  getEvents: (limit = 100, token) => request(`/api/events?limit=${limit}`, { token }),
  getNodes: (token) => request("/api/nodes", { token }),
  getNode: (nodeId, token) => request(`/api/nodes/${encodeURIComponent(nodeId)}`, { token }),
  getNetworkSummary: (token) => request("/api/network/summary", { token }),

  // Manual Ingestion & Prediction
  ingestReading: (payload, token) =>
    request("/api/ingest", {
      method: "POST",
      body: JSON.stringify(payload),
      token,
    }),
  predictRisk: (payload, token) =>
    request("/api/predict", {
      method: "POST",
      body: JSON.stringify(payload),
      token,
    }),

  // AI Chat & Conversations
  askChat: (question, conversationId, token) =>
    request("/api/chat", {
      method: "POST",
      body: JSON.stringify({ question, conversationId: conversationId || undefined }),
      token,
    }),
  getConversations: (token) => request("/api/conversations", { token }),
  getConversation: (id, token) => request(`/api/conversations/${encodeURIComponent(id)}`, { token }),
  deleteConversation: (id, token) =>
    request(`/api/conversations/${encodeURIComponent(id)}`, {
      method: "DELETE",
      token,
    }),

  // Admin Only Endpoints
  getAdminStatus: (token) => request("/api/admin/status", { token }),
  getAdminUsers: (token) => request("/api/admin/users", { token }),
  updateAdminUser: (username, { role, active }, token) =>
    request(`/api/admin/users/${encodeURIComponent(username)}`, {
      method: "PATCH",
      body: JSON.stringify({ role, active }),
      token,
    }),
  getAdminConversations: (token) => request("/api/admin/conversations", { token }),
  deleteAdminConversation: (id, token) =>
    request(`/api/admin/conversations/${encodeURIComponent(id)}`, {
      method: "DELETE",
      token,
    }),
};
