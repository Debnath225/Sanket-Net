import React from "react";

export function formatHazardName(val) {
  if (!val) return "Normal";
  return String(val)
    .toLowerCase()
    .replace(/([A-Z])/g, " $1")
    .replace(/^./, (str) => str.toUpperCase());
}

export function getRiskLevel(score) {
  const num = Number(score || 0);
  if (num >= 75) return { level: "CRITICAL", tone: "red", label: "Critical Threat" };
  if (num >= 45) return { level: "WARNING", tone: "amber", label: "High Caution" };
  if (num >= 20) return { level: "ADVISORY", tone: "yellow", label: "Low Advisory" };
  return { level: "NORMAL", tone: "green", label: "Safe / Normal" };
}

export function HazardBadge({ hazard, risk }) {
  const riskInfo = getRiskLevel(risk);
  return (
    <span className={`badge hazard-badge tone-${riskInfo.tone}`}>
      <span className="badge-dot" />
      {formatHazardName(hazard)} ({Math.round(risk || 0)}%)
    </span>
  );
}

export function StatusBadge({ online }) {
  return (
    <span className={`badge status-badge ${online ? "tone-green" : "tone-gray"}`}>
      <span className={`badge-dot ${online ? "pulse" : ""}`} />
      {online ? "Online" : "Offline"}
    </span>
  );
}

export function SeverityBadge({ priority }) {
  const p = String(priority || "INFO").toUpperCase();
  let tone = "blue";
  if (p === "CRITICAL" || p === "HIGH") tone = "red";
  else if (p === "WARNING" || p === "MEDIUM") tone = "amber";
  else if (p === "LOW") tone = "yellow";

  return <span className={`badge tone-${tone}`}>{p}</span>;
}
