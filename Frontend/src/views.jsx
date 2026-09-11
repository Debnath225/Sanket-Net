import React, { useContext, useMemo, useState } from "react";
import {
  Activity,
  ArrowUpRight,
  Bell,
  BrainCircuit,
  Database,
  Gauge,
  GitBranch,
  Network,
  Radio,
  Server,
  Settings2,
  ShieldCheck,
  Siren,
} from "lucide-react";
import {
  Area,
  AreaChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import { LiveMap } from "./components/LiveMap.jsx";
import {
  AlertList,
  Empty,
  PanelHeading,
  Stat,
  ViewTitle,
} from "./components/shared.jsx";
import { DataContext } from "./App.jsx";
import { formatHazard, riskTone } from "./data.js";

export function OverviewView({ nodes, alerts, telemetry, connected }) {
  const stats = {
    online: nodes.filter((node) => node.online).length,
    averageRisk: nodes.length
      ? Math.round(
          nodes.reduce((sum, node) => sum + Number(node.risk || 0), 0) /
            nodes.length,
        )
      : 0,
  };
  const chartData = telemetry
    .slice(0, 10)
    .reverse()
    .map((item, index) => ({
      name: index + 1,
      risk: item.prediction?.risk || item.risk_score || 0,
    }));
  return (
    <>
      <section className="hero-row">
        <div>
          <p className="eyebrow accent">NATIONAL RESILIENCE NETWORK</p>
          <h2>
            See the signal
            <br />
            <em>before the crisis.</em>
          </h2>
          <p className="hero-copy">
            Live field intelligence from distributed sensors, secure LoRa mesh,
            AI predictions, and gateway delivery.
          </p>
        </div>
        <div className="hero-orbit">
          <div className="orbit-ring ring-one" />
          <div className="orbit-ring ring-two" />
          <div className="orbit-core">
            <ShieldCheck size={24} />
            <span>
              SECURE
              <br />
              MESH
            </span>
          </div>
        </div>
      </section>
      <div className="stats-grid">
        <Stat
          icon={Radio}
          label="Field nodes"
          value={nodes.length}
          detail={`${stats.online} reporting now`}
          tone="cyan"
        />
        <Stat
          icon={Siren}
          label="Active alerts"
          value={alerts.length}
          detail="Live backend state"
          tone="red"
        />
        <Stat
          icon={Gauge}
          label="Average risk"
          value={`${stats.averageRisk}%`}
          detail="Current node signals"
          tone="amber"
        />
        <Stat
          icon={Database}
          label="Telemetry"
          value={telemetry.length}
          detail={connected ? "Streaming" : "Waiting"}
          tone="green"
        />
      </div>
      <div className="content-grid">
        <section className="panel chart-panel">
          <PanelHeading
            icon={Activity}
            title="Risk signal"
            subtitle="Real backend predictions"
          />
          {chartData.length ? (
            <div className="chart-wrap">
              <ResponsiveContainer width="100%" height="100%">
                <AreaChart data={chartData}>
                  <CartesianGrid
                    stroke="#203346"
                    strokeDasharray="3 5"
                    vertical={false}
                  />
                  <XAxis
                    dataKey="name"
                    stroke="#66798c"
                    tickLine={false}
                    axisLine={false}
                    fontSize={10}
                  />
                  <YAxis
                    domain={[0, 100]}
                    stroke="#66798c"
                    tickLine={false}
                    axisLine={false}
                    fontSize={10}
                  />
                  <Tooltip
                    contentStyle={{
                      background: "#102130",
                      border: "1px solid #294158",
                      borderRadius: 8,
                    }}
                  />
                  <Area
                    type="monotone"
                    dataKey="risk"
                    stroke="#f0a35b"
                    strokeWidth={2.5}
                    fill="#f0a35b33"
                  />
                </AreaChart>
              </ResponsiveContainer>
            </div>
          ) : (
            <Empty text="Waiting for live telemetry" />
          )}
        </section>
        <section className="panel map-panel">
          <PanelHeading
            icon={Network}
            title="Field network"
            subtitle="Click a marker for node information"
          />
          <LiveMap nodes={nodes} />
        </section>
      </div>
      <div className="bottom-grid">
        <section className="panel">
          <PanelHeading
            icon={Bell}
            title="Priority alerts"
            subtitle="Live AI triage"
          />
          <AlertList alerts={alerts.slice(0, 4)} formatHazard={formatHazard} />
        </section>
        <section className="panel">
          <PanelHeading
            icon={GitBranch}
            title="Pipeline health"
            subtitle="Gateway to backend delivery"
          />
          <div className="pipeline">
            <span>LoRa</span>
            <span>›</span>
            <span>Verify</span>
            <span>›</span>
            <span>AI</span>
            <span>›</span>
            <span className={connected ? "online" : ""}>MQTT</span>
          </div>
        </section>
      </div>
    </>
  );
}

export function NodesView({ nodes }) {
  const [query, setQuery] = useState("");
  const filtered = nodes.filter((node) =>
    `${node.id} ${node.role}`.toLowerCase().includes(query.toLowerCase()),
  );
  return (
    <ViewTitle
      eyebrow="OPERATIONS"
      title="Field nodes"
      description="Read-only live registry from gateway telemetry."
    >
      <div className="toolbar">
        <input
          className="input"
          value={query}
          onChange={(event) => setQuery(event.target.value)}
          placeholder="Search node or role"
        />
        <span className="muted">{filtered.length} live stations</span>
      </div>
      <div className="table-panel">
        <table>
          <thead>
            <tr>
              <th>Node</th>
              <th>Role</th>
              <th>Battery</th>
              <th>Risk</th>
              <th>Hazard</th>
              <th>Status</th>
            </tr>
          </thead>
          <tbody>
            {filtered.map((node) => (
              <tr key={node.id}>
                <td>
                  <strong>{node.id}</strong>
                  <small>
                    {Number.isFinite(node.lat)
                      ? `${node.lat.toFixed(4)}, ${node.lon.toFixed(4)}`
                      : "GPS unavailable"}
                  </small>
                </td>
                <td>{node.role}</td>
                <td>{node.battery ?? "-"}%</td>
                <td>
                  <span className={`risk-text ${riskTone(node.risk)}`}>
                    {Math.round(node.risk || 0)}%
                  </span>
                </td>
                <td>
                  <span className={`hazard-tag ${riskTone(node.risk)}`}>
                    {formatHazard(node.hazard)}
                  </span>
                </td>
                <td>
                  <span className="online">
                    <span
                      className={`status-dot ${node.online ? "good" : "bad"}`}
                    />
                    {node.online ? "Online" : "Offline"}
                  </span>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
        {!filtered.length && <Empty text="No live nodes reported" />}
      </div>
    </ViewTitle>
  );
}

export function NetworkView({ nodes, logs }) {
  return (
    <ViewTitle
      eyebrow="NETWORK INTELLIGENCE"
      title="Mesh network"
      description="Authenticated LoRa routes and gateway delivery state."
    >
      <div className="content-grid">
        <section className="panel network-map">
          <PanelHeading
            icon={Network}
            title="Topology"
            subtitle="Live GPS node positions"
          />
          <LiveMap nodes={nodes} />
        </section>
        <section className="panel log-panel">
          <PanelHeading
            icon={Activity}
            title="Packet log"
            subtitle="Live system events"
          />
          {logs.length ? (
            logs.map((log) => (
              <div className="log-line" key={log}>
                {log}
              </div>
            ))
          ) : (
            <Empty text="Waiting for gateway events" />
          )}
        </section>
      </div>
    </ViewTitle>
  );
}

export function AlertsView({ alerts }) {
  return (
    <ViewTitle
      eyebrow="INCIDENT RESPONSE"
      title="Alerts & events"
      description="Live hazards received from the backend AI pipeline."
    >
      <div className="panel table-panel">
        <table>
          <thead>
            <tr>
              <th>Time</th>
              <th>Node</th>
              <th>Hazard</th>
              <th>Priority</th>
              <th>Risk</th>
              <th>Confidence</th>
            </tr>
          </thead>
          <tbody>
            {alerts.map((alert, index) => (
              <tr key={`${alert.node_id}-${index}`}>
                <td>
                  {new Date(
                    alert.createdAt || alert.receivedAt || Date.now(),
                  ).toLocaleTimeString()}
                </td>
                <td>
                  <strong>{alert.node_id}</strong>
                </td>
                <td>{formatHazard(alert.hazard)}</td>
                <td>{alert.priority || "AI"}</td>
                <td>
                  <span className={`risk-text ${riskTone(alert.risk)}`}>
                    {Math.round(alert.risk || 0)}%
                  </span>
                </td>
                <td>{alert.confidence || 0}%</td>
              </tr>
            ))}
          </tbody>
        </table>
        {!alerts.length && <Empty text="No live alerts" />}
      </div>
    </ViewTitle>
  );
}

export function TelemetryView({ telemetry, nodes }) {
  const [node, setNode] = useState("all");
  const records = telemetry.filter(
    (item) => node === "all" || item.node_id === node,
  );
  return (
    <ViewTitle
      eyebrow="OBSERVABILITY"
      title="Live telemetry"
      description="Decoded gateway records and AI prediction context."
    >
      <div className="toolbar">
        <select
          className="input select"
          value={node}
          onChange={(event) => setNode(event.target.value)}
        >
          <option value="all">All nodes</option>
          {nodes.map((item) => (
            <option key={item.id}>{item.id}</option>
          ))}
        </select>
        <span className="muted">{records.length} live records</span>
      </div>
      <div className="panel table-panel">
        <table>
          <thead>
            <tr>
              <th>Received</th>
              <th>Node</th>
              <th>Temperature</th>
              <th>Humidity</th>
              <th>Water</th>
              <th>Soil</th>
              <th>AI risk</th>
            </tr>
          </thead>
          <tbody>
            {records.map((item, index) => (
              <tr key={`${item.receivedAt}-${index}`}>
                <td>
                  {new Date(item.receivedAt || Date.now()).toLocaleTimeString()}
                </td>
                <td>
                  <strong>{item.node_id}</strong>
                </td>
                <td>{item.temperature ?? "-"}</td>
                <td>{item.humidity ?? "-"}%</td>
                <td>{item.water_level ?? "-"}</td>
                <td>{item.soil_moisture ?? "-"}</td>
                <td>
                  <span
                    className={`risk-text ${riskTone(item.prediction?.risk || item.risk_score)}`}
                  >
                    {Math.round(item.prediction?.risk || item.risk_score || 0)}%
                  </span>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
        {!records.length && <Empty text="Waiting for MQTT telemetry" />}
      </div>
    </ViewTitle>
  );
}

export function EventsView({ events }) {
  return (
    <ViewTitle
      eyebrow="SYSTEM ACTIVITY"
      title="Events"
      description="Gateway, MQTT, and backend events from the live operational stream."
    >
      <section className="panel event-panel">
        {events.length ? (
          events.map((event, index) => (
            <article className="event-row" key={`${event.at || event.createdAt}-${index}`}>
              <span className={`event-level ${event.level || "info"}`} />
              <div>
                <strong>{event.message || "System event"}</strong>
                <p>{event.data?.node_id || event.node_id || "Sankat-Net service"}</p>
              </div>
              <time>{new Date(event.at || event.createdAt || Date.now()).toLocaleTimeString()}</time>
            </article>
          ))
        ) : (
          <Empty text="Waiting for gateway and system events" />
        )}
      </section>
    </ViewTitle>
  );
}

export function SettingsView({ connected }) {
  const context = useContext(DataContext);
  const apiUrl = import.meta.env.VITE_BACKEND_URL || "http://localhost:4000";
  return (
    <ViewTitle
      eyebrow="SYSTEM CONFIGURATION"
      title="Settings"
      description="Runtime connection and security status for this dashboard."
      action={
        <button className="button primary" onClick={context?.refresh}>
          <Activity size={14} /> Refresh connection
        </button>
      }
    >
      <div className="settings-grid">
        <section className="panel settings-card">
          <PanelHeading
            icon={Settings2}
            title="Backend connection"
            subtitle="Read-only runtime status"
          />
          <div className="setting-row">
            <span>API endpoint</span>
            <strong className="endpoint">{apiUrl}</strong>
          </div>
          <div className="setting-row">
            <span>WebSocket</span>
            <strong className={`connection-state ${connected ? "connected" : ""}`}>
              <i className={`status-dot ${connected ? "good" : "bad"}`} />
              {connected ? "Connected" : "Disconnected"}
            </strong>
          </div>
          <div className="setting-row">
            <span>Data source</span>
            <strong>MQTT → Node.js → Prisma</strong>
          </div>
        </section>
        <section className="panel settings-card">
          <PanelHeading
            icon={ShieldCheck}
            title="Security posture"
            subtitle="Transport and authentication"
          />
          <div className="setting-row">
            <span>MQTT transport</span>
            <strong>TLS required</strong>
          </div>
          <div className="setting-row">
            <span>Node packets</span>
            <strong>AES-GCM verified</strong>
          </div>
          <div className="setting-row">
            <span>AI model</span>
            <strong>Explainable threshold v1</strong>
          </div>
        </section>
      </div>
      <p className="settings-note">
        Connection addresses are supplied when the dashboard is built. Set
        <code> VITE_BACKEND_URL </code> and rebuild to use another backend.
      </p>
    </ViewTitle>
  );
}

export function NotFoundView() {
  return (
    <ViewTitle
      eyebrow="404"
      title="Page not found"
      description="Use the navigation to return to a live operational view."
    />
  );
}
