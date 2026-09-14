import React, { useState } from "react";
import { Activity, ArrowDownUp, CheckCircle, Network, Radio, Server, Wifi } from "lucide-react";
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { Panel } from "../../components/common/Panel.jsx";
import { StatCard } from "../../components/common/StatCard.jsx";
import { LiveMap } from "../../components/maps/LiveMap.jsx";
import { EmptyState } from "../../components/common/EmptyState.jsx";

export function AdminNetwork() {
  const { nodes, logs, connected } = useLiveData();
  const [filterQuery, setFilterQuery] = useState("");

  const filteredLogs = logs.filter((log) =>
    log.toLowerCase().includes(filterQuery.toLowerCase()),
  );

  return (
    <div className="admin-page-view">
      <div className="admin-kpi-grid">
        <StatCard
          icon={Network}
          label="Mesh Routing Hops"
          value={nodes.length > 0 ? `${nodes.length} Nodes` : "Standby"}
          detail="Store-and-forward mesh topology"
          tone="cyan"
        />
        <StatCard
          icon={Server}
          label="Gateway Link"
          value={connected ? "Active / Online" : "Reconnecting"}
          detail="MQTT over TLS on port 8883"
          tone={connected ? "green" : "red"}
        />
        <StatCard
          icon={ArrowDownUp}
          label="Packet Stream Buffer"
          value={`${logs.length} Packets`}
          detail="In-memory ring buffer"
          tone="purple"
        />
      </div>

      <div className="admin-split-grid">
        <Panel
          icon={Network}
          title="LoRa Mesh Geospatial Topology"
          subtitle="Real-time radio link nodes and gateway coverage coordinates"
        >
          <LiveMap nodes={nodes} height="420px" />
        </Panel>

        <Panel
          icon={Activity}
          title="Live LoRa Packet Stream"
          subtitle="Real-time decoded incoming gateway frame logs"
          action={
            <input
              type="text"
              className="admin-input-small"
              value={filterQuery}
              onChange={(e) => setFilterQuery(e.target.value)}
              placeholder="Filter packet text..."
            />
          }
        >
          <div className="packet-terminal-container">
            {filteredLogs.length > 0 ? (
              <div className="packet-log-stream">
                {filteredLogs.map((log, index) => (
                  <div key={index} className="terminal-line">
                    <span className="terminal-prefix">[FRAME]</span>
                    <span className="terminal-text">{log}</span>
                  </div>
                ))}
              </div>
            ) : (
              <EmptyState
                icon={Activity}
                title="Awaiting Gateway Frames"
                description="Live decrypted LoRa packets will scroll here in real time as nodes transmit."
              />
            )}
          </div>
        </Panel>
      </div>
    </div>
  );
}
