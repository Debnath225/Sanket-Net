import React, { useState } from "react";
import { Activity, Database, Droplets, Filter, Gauge, RefreshCw, Thermometer } from "lucide-react";
import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { Panel } from "../../components/common/Panel.jsx";
import { EmptyState } from "../../components/common/EmptyState.jsx";
import { HazardBadge } from "../../components/common/Badge.jsx";

export function AdminTelemetry() {
  const { telemetry, nodes, refresh } = useLiveData();
  const [selectedNode, setSelectedNode] = useState("all");

  const filteredTelemetry = telemetry.filter(
    (item) => selectedNode === "all" || item.node_id === selectedNode,
  );

  const chartData = filteredTelemetry
    .slice(0, 20)
    .reverse()
    .map((item, idx) => ({
      index: idx + 1,
      time: new Date(item.receivedAt || Date.now()).toLocaleTimeString([], {
        hour: "2-digit",
        minute: "2-digit",
      }),
      water: Number(item.water_level || 0),
      soil: Number(item.soil_moisture || 0),
      temp: Number(item.temperature || 0),
      rain: Number(item.rainfall_mm || 0),
    }));

  return (
    <div className="admin-page-view">
      <Panel
        icon={Activity}
        title="Multi-Parameter Environmental Telemetry"
        subtitle="Time-series observations decoded from incoming LoRa packets"
        action={
          <div className="admin-toolbar">
            <select
              className="admin-select"
              value={selectedNode}
              onChange={(e) => setSelectedNode(e.target.value)}
            >
              <option value="all">All Field Stations</option>
              {nodes.map((node) => (
                <option key={node.id} value={node.id}>
                  Station {node.id}
                </option>
              ))}
            </select>

            <button className="admin-btn-icon" onClick={refresh} title="Refresh records">
              <RefreshCw size={15} />
            </button>
          </div>
        }
      >
        {chartData.length > 0 ? (
          <div className="telemetry-chart-container">
            <ResponsiveContainer width="100%" height={280}>
              <LineChart data={chartData} margin={{ top: 10, right: 20, left: -15, bottom: 0 }}>
                <CartesianGrid stroke="#1e293b" strokeDasharray="3 3" vertical={false} />
                <XAxis
                  dataKey="time"
                  stroke="#64748b"
                  fontSize={11}
                  tickLine={false}
                  axisLine={false}
                />
                <YAxis
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
                <Legend />
                <Line
                  type="monotone"
                  dataKey="water"
                  name="Water Level (cm)"
                  stroke="#38bdf8"
                  strokeWidth={2}
                  dot={{ r: 3 }}
                />
                <Line
                  type="monotone"
                  dataKey="soil"
                  name="Soil Moisture (%)"
                  stroke="#10b981"
                  strokeWidth={2}
                  dot={{ r: 3 }}
                />
                <Line
                  type="monotone"
                  dataKey="temp"
                  name="Temp (°C)"
                  stroke="#f59e0b"
                  strokeWidth={2}
                  dot={{ r: 3 }}
                />
                <Line
                  type="monotone"
                  dataKey="rain"
                  name="Rainfall (mm)"
                  stroke="#a855f7"
                  strokeWidth={2}
                  dot={{ r: 3 }}
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
        ) : (
          <EmptyState
            icon={Activity}
            title="No Sensor Readings Logged"
            description="Readings will graph dynamically as MQTT telemetry arrives."
          />
        )}
      </Panel>

      <Panel
        icon={Database}
        title="Decoded Sensor Telemetry Log"
        subtitle={`${filteredTelemetry.length} recent telemetry transmissions`}
      >
        {filteredTelemetry.length > 0 ? (
          <div className="admin-table-wrapper">
            <table className="admin-table">
              <thead>
                <tr>
                  <th>Received Time</th>
                  <th>Node ID</th>
                  <th>Temperature</th>
                  <th>Humidity</th>
                  <th>Water Level</th>
                  <th>Soil Moisture</th>
                  <th>Rainfall</th>
                  <th>AI Risk Score</th>
                </tr>
              </thead>
              <tbody>
                {filteredTelemetry.map((item, index) => (
                  <tr key={`${item.receivedAt}-${index}`}>
                    <td>
                      {new Date(item.receivedAt || Date.now()).toLocaleTimeString()}
                    </td>
                    <td>
                      <strong>{item.node_id}</strong>
                    </td>
                    <td>{item.temperature !== undefined ? `${item.temperature}°C` : "—"}</td>
                    <td>{item.humidity !== undefined ? `${item.humidity}%` : "—"}</td>
                    <td>{item.water_level !== undefined ? `${item.water_level} cm` : "—"}</td>
                    <td>{item.soil_moisture !== undefined ? `${item.soil_moisture}%` : "—"}</td>
                    <td>{item.rainfall_mm !== undefined ? `${item.rainfall_mm} mm` : "—"}</td>
                    <td>
                      <HazardBadge
                        hazard={item.prediction?.hazard || item.hazard}
                        risk={item.prediction?.risk || item.risk_score}
                      />
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        ) : (
          <EmptyState
            icon={Database}
            title="Waiting for Telemetry Packets"
            description="MQTT gateway is listening on topic root 'sankatnet/telemetry/#'"
          />
        )}
      </Panel>
    </div>
  );
}
