export const API_URL =
  import.meta.env.VITE_BACKEND_URL || "http://localhost:3000";
export const WS_URL = API_URL.replace(/^http/, "ws") + "/ws";

export const adminNavigation = [
  ["/admin", "Overview", "LayoutDashboard"],
  ["/admin/nodes", "Field nodes", "Radio"],
  ["/admin/network", "Mesh network", "Network"],
  ["/admin/alerts", "Alerts", "Siren"],
  ["/admin/telemetry", "Telemetry", "Activity"],
  ["/admin/events", "Events", "Bell"],
  ["/admin/users", "Users", "Users"],
  ["/admin/settings", "Settings", "Settings2"],
];

export const userNavigation = [
  ["/user", "Safety hub", "ShieldCheck"],
  ["/user/alerts", "Safety alerts", "Siren"],
  ["/user/map", "Shelters map", "MapPin"],
  ["/user/guidance", "Go-bag guide", "BookOpen"],
  ["/user/report", "Report hazard", "Send"],
  ["/user/sos", "Emergency SOS", "PhoneCall"],
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
  (records || []).forEach((record) => {
    if (!record) return;
    const id = String(record.node_id || record.source_node || "UNKNOWN");
    const previous = merged.get(id) || {
      id,
      role: "Field station",
      battery: null,
      lat: null,
      lon: null,
    };
    merged.set(id, {
      ...previous,
      online: true,
      risk: Number(record.prediction?.risk ?? record.risk_score ?? previous.risk ?? 0),
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
      temperature: record.temperature ?? previous.temperature ?? null,
      humidity: record.humidity ?? previous.humidity ?? null,
      water_level: record.water_level ?? previous.water_level ?? null,
      soil_moisture: record.soil_moisture ?? previous.soil_moisture ?? null,
      rainfall_mm: record.rainfall_mm ?? previous.rainfall_mm ?? null,
      lastSeen: record.receivedAt || new Date().toISOString(),
    });
  });
  return [...merged.values()];
}
