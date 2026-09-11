import React from "react";
import {
  Activity,
  ArrowUpRight,
  Bell,
  BrainCircuit,
  CircleDot,
  CloudRain,
  Database,
  Gauge,
  GitBranch,
  LayoutDashboard,
  Network,
  Radio,
  Server,
  Settings2,
  ShieldCheck,
  Siren,
} from "lucide-react";

export const icons = {
  Activity,
  Bell,
  BrainCircuit,
  CloudRain,
  Database,
  Gauge,
  GitBranch,
  LayoutDashboard,
  Network,
  Radio,
  Server,
  Settings2,
  ShieldCheck,
  Siren,
};
export const markerColor = (risk) =>
  Number(risk || 0) >= 75
    ? "#f27678"
    : Number(risk || 0) >= 45
      ? "#f1ad62"
      : "#78d6a4";
export { ArrowUpRight, CircleDot };
export function Empty({ text }) {
  return (
    <div className="empty">
      <Database size={18} />
      <span>{text}</span>
    </div>
  );
}
export function PanelHeading({ icon: Icon, title, subtitle, action }) {
  return (
    <div className="panel-heading">
      <div className="heading-icon">
        <Icon size={16} />
      </div>
      <div>
        <h3>{title}</h3>
        <p>{subtitle}</p>
      </div>
      {action && <div className="heading-action">{action}</div>}
    </div>
  );
}
export function ViewTitle({ eyebrow, title, description, action, children }) {
  return (
    <>
      <div className="view-title">
        <div>
          <p className="eyebrow accent">{eyebrow}</p>
          <h2>{title}</h2>
          <p>{description}</p>
        </div>
        {action}
      </div>
      {children}
    </>
  );
}
export function AlertList({ alerts, formatHazard }) {
  return (
    <div className="alert-list">
      {alerts.length ? (
        alerts.map((alert, index) => (
          <div className="alert-row" key={`${alert.node_id}-${index}`}>
            <span
              className={`alert-severity ${alert.priority === "CRITICAL" ? "critical" : "warning"}`}
            />
            <div>
              <strong>{formatHazard(alert.hazard)}</strong>
              <p>
                {alert.node_id} · {alert.confidence || 0}% confidence
              </p>
            </div>
            <b>{Math.round(alert.risk || 0)}%</b>
          </div>
        ))
      ) : (
        <Empty text="No live alerts" />
      )}
    </div>
  );
}
export function Stat({ icon: Icon, label, value, detail, tone }) {
  return (
    <div className={`stat-card ${tone}`}>
      <div className="stat-icon">
        <Icon size={16} />
      </div>
      <div>
        <p>{label}</p>
        <strong>{value}</strong>
        <span>{detail}</span>
      </div>
    </div>
  );
}
