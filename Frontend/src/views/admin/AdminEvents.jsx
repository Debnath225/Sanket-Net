import React, { useState } from "react";
import { AlertCircle, AlertTriangle, Bell, CheckCircle2, Clock, Info, ShieldCheck } from "lucide-react";
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { Panel } from "../../components/common/Panel.jsx";
import { EmptyState } from "../../components/common/EmptyState.jsx";

export function AdminEvents() {
  const { events } = useLiveData();
  const [levelFilter, setLevelFilter] = useState("all");

  const filteredEvents = events.filter((ev) => {
    if (levelFilter === "all") return true;
    return (ev.level || "info").toLowerCase() === levelFilter.toLowerCase();
  });

  const getLevelIcon = (lvl) => {
    const l = String(lvl || "info").toLowerCase();
    if (l === "error" || l === "critical") return <AlertCircle size={16} className="text-red" />;
    if (l === "warning" || l === "warn") return <AlertTriangle size={16} className="text-amber" />;
    return <Info size={16} className="text-cyan" />;
  };

  return (
    <div className="admin-page-view">
      <Panel
        icon={Bell}
        title="Operational System Events & Audit Trail"
        subtitle="Gateway packet routing, connection transitions, and infrastructure warnings"
        action={
          <div className="admin-toolbar">
            <select
              className="admin-select"
              value={levelFilter}
              onChange={(e) => setLevelFilter(e.target.value)}
            >
              <option value="all">All Severities ({events.length})</option>
              <option value="info">Info</option>
              <option value="warning">Warning</option>
              <option value="error">Error / Fault</option>
            </select>
          </div>
        }
      >
        {filteredEvents.length > 0 ? (
          <div className="event-stream-container">
            {filteredEvents.map((ev, index) => (
              <div
                key={`${ev.at || ev.createdAt}-${index}`}
                className={`event-item-card level-${ev.level || "info"}`}
              >
                <div className="event-level-icon">{getLevelIcon(ev.level)}</div>
                <div className="event-body">
                  <div className="event-head">
                    <strong className="event-message">{ev.message || "Operational Event"}</strong>
                    <span className="event-source">
                      {ev.data?.node_id || ev.node_id || "Mesh Gateway"}
                    </span>
                  </div>
                  {ev.data && typeof ev.data === "object" && (
                    <code className="event-payload">
                      {JSON.stringify(ev.data)}
                    </code>
                  )}
                </div>
                <div className="event-time">
                  <Clock size={12} />
                  <span>{new Date(ev.at || ev.createdAt || Date.now()).toLocaleTimeString()}</span>
                </div>
              </div>
            ))}
          </div>
        ) : (
          <EmptyState
            icon={Bell}
            title="No Events Recorded"
            description="Operational events and gateway heartbeat transitions will appear here."
          />
        )}
      </Panel>
    </div>
  );
}
