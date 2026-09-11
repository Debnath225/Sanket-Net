export const API_URL =
  import.meta.env.VITE_BACKEND_URL || "http://localhost:4000";
export const WS_URL = API_URL.replace(/^http/, "ws") + "/ws";

export const navigation = [
  ["/", "Overview", "LayoutDashboard"],
  ["/nodes", "Field nodes", "Radio"],
  ["/network", "Mesh network", "Network"],
  ["/alerts", "Alerts", "Siren"],
  ["/telemetry", "Telemetry", "Activity"],
  ["/events", "Events", "Bell"],
  ["/settings", "Settings", "Settings2"],
];

export const emptySnapshot = {
  telemetry: [],
  alerts: [],
  heartbeats: [],
  events: [],
};

export function riskTone(risk) {
  const value = Number(risk || 0);
  return value >= 75 ? "high" : value >= 45 ? "medium" : "low";
}

export function formatHazard(value) {
  return String(value || "normal")
    .replace(/([A-Z])/g, " $1")
    .replace(/^./, (letter) => letter.toUpperCase());
}

export function mergeNodes(current, records) {
  const merged = new Map(current.map((node) => [node.id, node]));
  records.forEach((record) => {
    const id = String(record.node_id || record.source_node || "UNKNOWN");
    const previous = merged.get(id) || {
      id,
      role: "Field node",
      battery: null,
    };
    merged.set(id, {
      ...previous,
      online: true,
      risk: record.prediction?.risk ?? record.risk_score ?? previous.risk ?? 0,
      hazard: String(
        record.prediction?.hazard ||
          record.hazard ||
          previous.hazard ||
          "NORMAL",
      ).toUpperCase(),
      lat: Number(record.latitude ?? previous.lat),
      lon: Number(record.longitude ?? previous.lon),
      battery: record.battery_mv
        ? Math.min(100, Math.round(Number(record.battery_mv) / 42))
        : previous.battery,
      lastSeen: record.receivedAt || new Date().toISOString(),
    });
  });
  return [...merged.values()];
}
