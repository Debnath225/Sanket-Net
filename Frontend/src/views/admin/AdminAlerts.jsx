import React, { useState } from "react";
import { AlertCircle, Bell, Filter, Plus, Send, ShieldAlert, Siren } from "lucide-react";
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { useAuth } from "../../context/AuthContext.jsx";
import { Panel } from "../../components/common/Panel.jsx";
import { HazardBadge, SeverityBadge, formatHazardName } from "../../components/common/Badge.jsx";
import { EmptyState } from "../../components/common/EmptyState.jsx";
import { Modal } from "../../components/common/Modal.jsx";
import { endpoints } from "../../api/endpoints.js";

export function AdminAlerts() {
  const { alerts, refresh } = useLiveData();
  const { session } = useAuth();
  const [filterSeverity, setFilterSeverity] = useState("all");
  const [broadcastOpen, setBroadcastOpen] = useState(false);
  const [broadcasting, setBroadcasting] = useState(false);
  const [broadcastSuccess, setBroadcastSuccess] = useState(false);

  const [formHazard, setFormHazard] = useState("flood");
  const [formNodeId, setFormNodeId] = useState("COMMAND-01");
  const [formRisk, setFormRisk] = useState(85);
  const [formWater, setFormWater] = useState(720);

  const filteredAlerts = alerts.filter((alert) => {
    if (filterSeverity === "all") return true;
    const p = String(alert.priority || "INFO").toUpperCase();
    if (filterSeverity === "critical") return p === "CRITICAL" || Number(alert.risk || 0) >= 75;
    if (filterSeverity === "warning") return p === "WARNING" || (Number(alert.risk || 0) >= 45 && Number(alert.risk || 0) < 75);
    return true;
  });

  const handleBroadcastAlert = async (e) => {
    e.preventDefault();
    if (!session?.token) return;
    setBroadcasting(true);
    try {
      await endpoints.ingestReading(
        {
          node_id: formNodeId,
          hazard: formHazard,
          water_level: Number(formWater),
          rainfall_mm: 45,
          temperature: 27,
          humidity: 88,
          battery_mv: 4100,
          latitude: 22.5726,
          longitude: 88.3639,
        },
        session.token,
      );
      setBroadcastSuccess(true);
      setTimeout(() => {
        setBroadcastSuccess(false);
        setBroadcastOpen(false);
        refresh();
      }, 1200);
    } catch {
      alert("Failed to broadcast alert. Verify backend is running.");
    } finally {
      setBroadcasting(false);
    }
  };

  return (
    <div className="admin-page-view">
      <Panel
        icon={Siren}
        title="Active Incident Alerts & Triage"
        subtitle="AI neural risk classifications and active emergency notices"
        action={
          <div className="admin-toolbar">
            <select
              className="admin-select"
              value={filterSeverity}
              onChange={(e) => setFilterSeverity(e.target.value)}
            >
              <option value="all">All Severities ({alerts.length})</option>
              <option value="critical">Critical (Risk ≥ 75%)</option>
              <option value="warning">Warning (Risk ≥ 45%)</option>
            </select>

            <button
              className="admin-primary-btn"
              onClick={() => setBroadcastOpen(true)}
            >
              <Plus size={16} />
              <span>Simulate / Dispatch Alert</span>
            </button>
          </div>
        }
      >
        {filteredAlerts.length > 0 ? (
          <div className="admin-table-wrapper">
            <table className="admin-table">
              <thead>
                <tr>
                  <th>Timestamp</th>
                  <th>Origin Station</th>
                  <th>Hazard Classification</th>
                  <th>Priority</th>
                  <th>Risk Score</th>
                  <th>Neural Confidence</th>
                </tr>
              </thead>
              <tbody>
                {filteredAlerts.map((alert, index) => (
                  <tr key={`${alert.node_id}-${index}`}>
                    <td>
                      {new Date(
                        alert.createdAt || alert.receivedAt || Date.now(),
                      ).toLocaleTimeString()}
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
                    <td>
                      <span className="confidence-pill">
                        {alert.confidence || 75}%
                      </span>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        ) : (
          <EmptyState
            icon={Siren}
            title="No Incidents Currently Reported"
            description="Environmental telemetry is stable. No critical flood, landslide, or fire thresholds breached."
          />
        )}
      </Panel>

      {/* Manual Alert Ingestion Modal */}
      <Modal
        isOpen={broadcastOpen}
        onClose={() => setBroadcastOpen(false)}
        title="Simulate / Dispatch Emergency Alert"
        subtitle="Submit telemetry to the live AI inference pipeline and trigger alerts"
      >
        <form onSubmit={handleBroadcastAlert} className="admin-form">
          <div className="form-group">
            <label>Origin Node ID</label>
            <input
              type="text"
              value={formNodeId}
              onChange={(e) => setFormNodeId(e.target.value)}
              required
            />
          </div>

          <div className="form-group">
            <label>Hazard Type</label>
            <select
              value={formHazard}
              onChange={(e) => setFormHazard(e.target.value)}
            >
              <option value="flood">Flash Flood</option>
              <option value="landslide">Landslide / Soil Instability</option>
              <option value="fire">Wildfire / Heat Anomaly</option>
              <option value="pollution">Air Contamination</option>
            </select>
          </div>

          <div className="form-row">
            <div className="form-group">
              <label>Water Level (cm)</label>
              <input
                type="number"
                value={formWater}
                onChange={(e) => setFormWater(e.target.value)}
                min="0"
                max="2000"
              />
            </div>
            <div className="form-group">
              <label>Risk Probability Target (%)</label>
              <input
                type="number"
                value={formRisk}
                onChange={(e) => setFormRisk(e.target.value)}
                min="10"
                max="99"
              />
            </div>
          </div>

          <button
            type="submit"
            className="admin-primary-btn full"
            disabled={broadcasting}
          >
            {broadcasting
              ? "Dispatching..."
              : broadcastSuccess
                ? "Incident Dispatched!"
                : "Submit Telemetry Frame to Pipeline"}
          </button>
        </form>
      </Modal>
    </div>
  );
}
