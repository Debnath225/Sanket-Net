import React, { useState } from "react";
import { Battery, BatteryCharging, Droplets, Gauge, MapPin, Radio, Search, Thermometer, Wifi } from "lucide-react";
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { Panel } from "../../components/common/Panel.jsx";
import { HazardBadge, StatusBadge, formatHazardName } from "../../components/common/Badge.jsx";
import { EmptyState } from "../../components/common/EmptyState.jsx";
import { Modal } from "../../components/common/Modal.jsx";

export function AdminNodes() {
  const { nodes } = useLiveData();
  const [search, setSearch] = useState("");
  const [filterOnline, setFilterOnline] = useState("all");
  const [selectedNode, setSelectedNode] = useState(null);

  const filteredNodes = nodes.filter((node) => {
    const matchesSearch =
      node.id.toLowerCase().includes(search.toLowerCase()) ||
      (node.hazard && node.hazard.toLowerCase().includes(search.toLowerCase()));

    const matchesStatus =
      filterOnline === "all" ||
      (filterOnline === "online" && node.online) ||
      (filterOnline === "offline" && !node.online);

    return matchesSearch && matchesStatus;
  });

  return (
    <div className="admin-page-view">
      <Panel
        icon={Radio}
        title="Field Monitoring Stations"
        subtitle="Hardware telemetry registry, battery health, and sensor operational parameters"
        action={
          <div className="admin-toolbar">
            <div className="search-box">
              <Search size={15} />
              <input
                type="text"
                value={search}
                onChange={(e) => setSearch(e.target.value)}
                placeholder="Search station ID or hazard..."
              />
            </div>

            <select
              className="admin-select"
              value={filterOnline}
              onChange={(e) => setFilterOnline(e.target.value)}
            >
              <option value="all">All Stations ({nodes.length})</option>
              <option value="online">Online Only</option>
              <option value="offline">Offline Only</option>
            </select>
          </div>
        }
      >
        {filteredNodes.length > 0 ? (
          <div className="admin-table-wrapper">
            <table className="admin-table">
              <thead>
                <tr>
                  <th>Station ID</th>
                  <th>Coordinates</th>
                  <th>Battery</th>
                  <th>Water Level</th>
                  <th>Soil Moisture</th>
                  <th>Hazard Assessment</th>
                  <th>Status</th>
                  <th>Action</th>
                </tr>
              </thead>
              <tbody>
                {filteredNodes.map((node) => (
                  <tr key={node.id}>
                    <td>
                      <div className="node-id-cell">
                        <Radio size={14} className="node-icon" />
                        <strong>{node.id}</strong>
                      </div>
                    </td>
                    <td>
                      <span className="coord-text">
                        {Number.isFinite(node.lat)
                          ? `${node.lat.toFixed(4)}, ${node.lon.toFixed(4)}`
                          : "GPS Awaiting Lock"}
                      </span>
                    </td>
                    <td>
                      <div className="battery-indicator">
                        <Battery size={15} />
                        <span>{node.battery !== null ? `${node.battery}%` : "—"}</span>
                      </div>
                    </td>
                    <td>
                      <span>{node.water_level !== null ? `${node.water_level} cm` : "—"}</span>
                    </td>
                    <td>
                      <span>{node.soil_moisture !== null ? `${node.soil_moisture}%` : "—"}</span>
                    </td>
                    <td>
                      <HazardBadge hazard={node.hazard} risk={node.risk} />
                    </td>
                    <td>
                      <StatusBadge online={node.online} />
                    </td>
                    <td>
                      <button
                        className="admin-action-btn"
                        onClick={() => setSelectedNode(node)}
                      >
                        Inspect
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        ) : (
          <EmptyState
            icon={Radio}
            title="No Field Stations Found"
            description={
              nodes.length === 0
                ? "Waiting for field nodes to register via LoRa mesh gateway..."
                : "No stations match your current search and filter criteria."
            }
          />
        )}
      </Panel>

      {/* Node Detail Modal */}
      <Modal
        isOpen={Boolean(selectedNode)}
        onClose={() => setSelectedNode(null)}
        title={`Field Station Diagnostics: ${selectedNode?.id}`}
        subtitle="Hardware telemetry & environmental reading breakdown"
      >
        {selectedNode && (
          <div className="node-modal-content">
            <div className="node-stat-grid">
              <div className="modal-metric-card">
                <Thermometer size={16} />
                <span className="metric-label">Temperature</span>
                <strong>{selectedNode.temperature ? `${selectedNode.temperature}°C` : "N/A"}</strong>
              </div>
              <div className="modal-metric-card">
                <Droplets size={16} />
                <span className="metric-label">Water Level</span>
                <strong>{selectedNode.water_level ? `${selectedNode.water_level} cm` : "N/A"}</strong>
              </div>
              <div className="modal-metric-card">
                <Gauge size={16} />
                <span className="metric-label">Soil Moisture</span>
                <strong>{selectedNode.soil_moisture ? `${selectedNode.soil_moisture}%` : "N/A"}</strong>
              </div>
              <div className="modal-metric-card">
                <Battery size={16} />
                <span className="metric-label">Battery Power</span>
                <strong>{selectedNode.battery ? `${selectedNode.battery}%` : "N/A"}</strong>
              </div>
            </div>

            <div className="modal-section">
              <h4>Telemetry Metadata</h4>
              <div className="meta-list">
                <div className="meta-item">
                  <span>Status:</span>
                  <StatusBadge online={selectedNode.online} />
                </div>
                <div className="meta-item">
                  <span>Calculated Hazard:</span>
                  <HazardBadge hazard={selectedNode.hazard} risk={selectedNode.risk} />
                </div>
                <div className="meta-item">
                  <span>GPS Latitude / Longitude:</span>
                  <code>{selectedNode.lat ?? "N/A"}, {selectedNode.lon ?? "N/A"}</code>
                </div>
                <div className="meta-item">
                  <span>Last Seen Timestamp:</span>
                  <span>{new Date(selectedNode.lastSeen).toLocaleString()}</span>
                </div>
              </div>
            </div>
          </div>
        )}
      </Modal>
    </div>
  );
}
