import React from "react";
import { NavLink } from "react-router-dom";
import {
  Activity,
  Bell,
  ChevronRight,
  Database,
  GitBranch,
  LayoutDashboard,
  Network,
  Radio,
  Settings2,
  ShieldCheck,
  Siren,
  Users,
  Wifi,
  X,
} from "lucide-react";
import { useLiveData } from "../../context/LiveDataContext.jsx";

const adminNavItems = [
  { path: "/admin", label: "Command Overview", icon: LayoutDashboard, end: true },
  { path: "/admin/nodes", label: "Field Nodes", icon: Radio },
  { path: "/admin/network", label: "LoRa Mesh Network", icon: Network },
  { path: "/admin/alerts", label: "Incident Alerts", icon: Siren, badgeKey: "alerts" },
  { path: "/admin/telemetry", label: "Live Telemetry", icon: Activity },
  { path: "/admin/events", label: "System Events", icon: Bell },
  { path: "/admin/users", label: "User Access", icon: Users },
  { path: "/admin/settings", label: "System Diagnostics", icon: Settings2 },
];

export function AdminSidebar({ mobileOpen, onClose }) {
  const { connected, alerts } = useLiveData();
  const criticalCount = alerts.filter((a) => Number(a.risk || 0) >= 75).length;

  return (
    <>
      <aside className={`admin-sidebar ${mobileOpen ? "open" : ""}`}>
        <div className="sidebar-brand">
          <div className="brand-badge">SN</div>
          <div className="brand-info">
            <span className="brand-title">Sankat-Net</span>
            <span className="brand-subtitle">COMMAND CENTER</span>
          </div>
          <button className="sidebar-close" onClick={onClose} aria-label="Close menu">
            <X size={18} />
          </button>
        </div>

        <div className="sidebar-badge-row">
          <span className="admin-pill">ADMINISTRATOR</span>
          <span className={`live-dot-chip ${connected ? "online" : "offline"}`}>
            <span className="dot" />
            {connected ? "Mesh Uplink Live" : "Reconnecting"}
          </span>
        </div>

        <nav className="sidebar-nav">
          {adminNavItems.map((item) => {
            const Icon = item.icon;
            const badgeValue =
              item.badgeKey === "alerts" && criticalCount > 0 ? criticalCount : null;

            return (
              <NavLink
                key={item.path}
                to={item.path}
                end={item.end}
                onClick={onClose}
                className={({ isActive }) =>
                  `sidebar-link ${isActive ? "active" : ""}`
                }
              >
                <div className="sidebar-link-content">
                  <Icon size={17} className="sidebar-icon" />
                  <span>{item.label}</span>
                </div>
                {badgeValue ? (
                  <span className="sidebar-alert-badge">{badgeValue}</span>
                ) : (
                  <ChevronRight size={14} className="sidebar-arrow" />
                )}
              </NavLink>
            );
          })}
        </nav>

        <div className="sidebar-footer">
          <div className="gateway-status-box">
            <div className="gateway-status-head">
              <span className={`status-indicator ${connected ? "good" : "bad"}`} />
              <strong>Gateway GW-01</strong>
            </div>
            <p className="gateway-status-desc">
              {connected ? "LoRa Mesh Bridge Active" : "MQTT Uplink Disconnected"}
            </p>
            <div className="gateway-meta">
              <Wifi size={12} />
              <span>868 MHz · AES-GCM Encrypted</span>
            </div>
          </div>
          <div className="sidebar-credits">
            <small>Disaster Resilience & Early Warning System</small>
          </div>
        </div>
      </aside>

      {mobileOpen && <div className="sidebar-backdrop" onClick={onClose} />}
    </>
  );
}
