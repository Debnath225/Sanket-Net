import React from "react";
import { Link } from "react-router-dom";
import {
  Activity,
  AlertTriangle,
  ArrowUpRight,
  Bell,
  Cpu,
  Database,
  Gauge,
  GitBranch,
  Network,
  Radio,
  RefreshCw,
  ShieldAlert,
  ShieldCheck,
  Siren,
  Wifi,
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
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { StatCard } from "../../components/common/StatCard.jsx";
import { Panel } from "../../components/common/Panel.jsx";
import { EmptyState } from "../../components/common/EmptyState.jsx";
import { HazardBadge, SeverityBadge } from "../../components/common/Badge.jsx";
import { LiveMap } from "../../components/maps/LiveMap.jsx";

export function AdminOverview() {
  const { nodes, alerts, telemetry, connected, refresh } = useLiveData();

  const onlineNodes = nodes.filter((n) => n.online).length;
  const criticalAlerts = alerts.filter((a) => Number(a.risk || 0) >= 75).length;
  const avgRisk = nodes.length
    ? Math.round(nodes.reduce((sum, n) => sum + Number(n.risk || 0), 0) / nodes.length)
    : 0;

  // Chart data from recent telemetry records
  const chartData = telemetry
    .slice(0, 15)
    .reverse()
    .map((item, idx) => ({
      index: idx + 1,
      time: new Date(item.receivedAt || Date.now()).toLocaleTimeString([], {
        hour: "2-digit",
        minute: "2-digit",
      }),
      risk: Number(item.prediction?.risk ?? item.risk_score ?? 0),
      water: Number(item.water_level || 0),
      temp: Number(item.temperature || 0),
    }));

  return (
    <div className="admin-overview-view">
      {/* Executive Hero Banner */}
      <section className="admin-hero-banner">
        <div className="hero-left">
          <div className="hero-eyebrow">
            <span className="live-dot-pulse" />
            <span>NATIONAL RESILIENCE GRID · OPERATIONS</span>
          </div>
          <h2 className="hero-title">
            Early Signal Detection &amp; <em>Crisis Intelligence</em>
          </h2>
          <p className="hero-desc">
            Autonomous multi-hop LoRa mesh ingestion, edge sensor verification,
            and explainable AI hazard prediction.
          </p>
        </div>

        <div className="hero-right">
          <div className="mesh-core-badge">
            <div className="core-icon">
              <ShieldCheck size={28} />
            </div>
            <div className="core-details">
              <strong>Mesh Status</strong>
              <span>{connected ? "Gateway Uplink Synced" : "Awaiting Transport"}</span>
              <small>AES-128 GCM Verified</small>
            </div>
          </div>
        </div>
      </section>

      {/* KPI Stats Grid */}
      <div className="admin-kpi-grid">
        <StatCard
          icon={Radio}
          label="Active Field Nodes"
          value={nodes.length}
          detail={`${onlineNodes} transmitting telemetry`}
          tone="cyan"
        />
        <StatCard
          icon={Siren}
          label="Critical Hazard Alerts"
          value={criticalAlerts}
          detail={`${alerts.length} total active incidents`}
          tone={criticalAlerts > 0 ? "red" : "green"}
        />
        <StatCard
          icon={Gauge}
          label="Network Mean Risk"
          value={`${avgRisk}%`}
          detail={avgRisk > 50 ? "High Regional Threat" : "Normal Baseline"}
          tone={avgRisk > 60 ? "amber" : "green"}
        />
        <StatCard
          icon={Database}
          label="Telemetry Throughput"
          value={telemetry.length}
          detail={connected ? "Streaming over WebSocket" : "Standby"}
          tone="purple"
        />
      </div>

      {/* Main Operational Split */}
      <div className="admin-split-grid">
        {/* Risk Trend Chart */}
        <Panel
          icon={Activity}
          title="Predictive Hazard Signal"
          subtitle="Real-time AI probability scoring across recent station packets"
          action={
            <Link to="/admin/telemetry" className="panel-link-btn">
              <span>View Sensor Metrics</span>
              <ArrowUpRight size={14} />
            </Link>
          }
        >
          {chartData.length > 0 ? (
            <div className="overview-chart-wrapper">
              <ResponsiveContainer width="100%" height={260}>
                <AreaChart data={chartData} margin={{ top: 10, right: 10, left: -20, bottom: 0 }}>
                  <defs>
                    <linearGradient id="riskGrad" x1="0" y1="0" x2="0" y2="1">
                      <stop offset="5%" stopColor="#f59e0b" stopOpacity={0.4} />
                      <stop offset="95%" stopColor="#f59e0b" stopOpacity={0.0} />
                    </linearGradient>
                  </defs>
                  <CartesianGrid stroke="#1e293b" strokeDasharray="3 3" vertical={false} />
                  <XAxis
                    dataKey="time"
                    stroke="#64748b"
                    fontSize={11}
                    tickLine={false}
                    axisLine={false}
                  />
                  <YAxis
                    domain={[0, 100]}
                    stroke="#64748b"
                    fontSize={11}
                    tickLine={false}
                    axisLine={false}
                  />
                  <Tooltip
                    contentStyle={{
                      backgroundColor: "#0f172a",
                      borderColor: "#334155",
                      borderRadius: "8px",
                      color: "#f8fafc",
                    }}
                  />
                  <Area
                    type="monotone"
                    dataKey="risk"
                    name="Risk Probability (%)"
                    stroke="#f59e0b"
                    strokeWidth={2.5}
                    fillOpacity={1}
                    fill="url(#riskGrad)"
                  />
                </AreaChart>
              </ResponsiveContainer>
            </div>
          ) : (
            <EmptyState
              icon={Activity}
              title="Awaiting Sensor Telemetry"
              description="Sensor readings will dynamically plot here as field nodes transmit"
            />
          )}
        </Panel>

        {/* Live Field Map */}
        <Panel
          icon={Network}
          title="Field Topology & Coverage"
          subtitle="GPS positioning of active reporting mesh stations"
          action={
            <Link to="/admin/network" className="panel-link-btn">
              <span>Full Topology</span>
              <ArrowUpRight size={14} />
            </Link>
          }
        >
          <LiveMap nodes={nodes} height="260px" />
        </Panel>
      </div>

      {/* Bottom Queue: Alerts & Pipeline Status */}
      <div className="admin-bottom-grid">
        <Panel
          icon={Bell}
          title="Priority Incident Queue"
          subtitle="Active environmental anomalies identified by neural engine"
          action={
            <Link to="/admin/alerts" className="panel-link-btn">
              <span>Alerts Center</span>
              <ArrowUpRight size={14} />
            </Link>
          }
        >
          {alerts.length > 0 ? (
            <div className="incident-table-wrap">
              <table className="admin-table mini">
                <thead>
                  <tr>
                    <th>Time</th>
                    <th>Node ID</th>
                    <th>Hazard</th>
                    <th>Priority</th>
                    <th>Risk %</th>
                  </tr>
                </thead>
                <tbody>
                  {alerts.slice(0, 5).map((alert, idx) => (
                    <tr key={`${alert.node_id}-${idx}`}>
                      <td>
                        {new Date(
                          alert.createdAt || alert.receivedAt || Date.now(),
                        ).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" })}
                      </td>
                      <td>
                        <strong>{alert.node_id}</strong>
                      </td>
                      <td>
                        <HazardBadge hazard={alert.hazard} risk={alert.risk} />
                      </td>
                      <td>
                        <SeverityBadge priority={alert.priority} />
                      </td>
                      <td>
                        <span className="risk-metric">
                          {Math.round(alert.risk || 0)}%
                        </span>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          ) : (
            <EmptyState
              icon={ShieldCheck}
              title="No Active Emergency Incidents"
              description="All monitoring stations report within normal environmental thresholds"
            />
          )}
        </Panel>

        <Panel
          icon={GitBranch}
          title="Telemetry Ingestion Pipeline"
          subtitle="End-to-end transport and inference health"
        >
          <div className="pipeline-chain">
            <div className="pipeline-step completed">
              <span className="step-num">1</span>
              <div>
                <strong>Sensor Pod</strong>
                <small>BME280 / Water Level</small>
              </div>
            </div>
            <div className="pipeline-divider">›</div>
            <div className="pipeline-step completed">
              <span className="step-num">2</span>
              <div>
                <strong>LoRa Mesh</strong>
                <small>Multi-hop Routing</small>
              </div>
            </div>
            <div className="pipeline-divider">›</div>
            <div className="pipeline-step completed">
              <span className="step-num">3</span>
              <div>
                <strong>Gateway</strong>
                <small>AES-GCM Auth</small>
              </div>
            </div>
            <div className="pipeline-divider">›</div>
            <div className={`pipeline-step ${connected ? "completed" : "pending"}`}>
              <span className="step-num">4</span>
              <div>
                <strong>AI Engine</strong>
                <small>{connected ? "Streaming Live" : "Offline"}</small>
              </div>
            </div>
          </div>
        </Panel>
      </div>
    </div>
  );
}
