import React, { useEffect, useState } from "react";
import { CheckCircle2, Cpu, Database, Key, Lock, RefreshCw, Server, Settings2, ShieldCheck, Wifi } from "lucide-react";
import { useAuth } from "../../context/AuthContext.jsx";
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { endpoints } from "../../api/endpoints.js";
import { API_URL, WS_URL } from "../../api/client.js";
import { Panel } from "../../components/common/Panel.jsx";

export function AdminSettings() {
  const { session } = useAuth();
  const { connected, refresh } = useLiveData();
  const token = session?.token;
  const [adminStatus, setAdminStatus] = useState(null);
  const [loading, setLoading] = useState(false);

  const fetchStatus = async () => {
    if (!token) return;
    setLoading(true);
    try {
      const data = await endpoints.getAdminStatus(token);
      setAdminStatus(data);
    } catch {
      // Fallback display
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchStatus();
  }, [token]);

  return (
    <div className="admin-page-view">
      <div className="settings-cards-grid">
        {/* Connection Diagnostics */}
        <Panel
          icon={Server}
          title="Backend & Gateway Connectivity"
          subtitle="Real-time link status and persistence layer"
          action={
            <button
              className="admin-action-btn"
              onClick={() => {
                refresh();
                fetchStatus();
              }}
              disabled={loading}
            >
              <RefreshCw size={14} />
              <span>Test Ping</span>
            </button>
          }
        >
          <div className="settings-row">
            <span className="setting-name">REST API Endpoint</span>
            <code className="setting-val">{API_URL}</code>
          </div>
          <div className="settings-row">
            <span className="setting-name">WebSocket Telemetry Channel</span>
            <code className="setting-val">{WS_URL}</code>
          </div>
          <div className="settings-row">
            <span className="setting-name">Live Uplink Status</span>
            <span className={`status-pill ${connected ? "online" : "offline"}`}>
              {connected ? "Connected & Receiving Telemetry" : "Offline / Reconnecting"}
            </span>
          </div>
          <div className="settings-row">
            <span className="setting-name">Persistence Engine</span>
            <span className="badge tone-blue">
              {adminStatus?.persistence || "Auto-detecting"}
            </span>
          </div>
          <div className="settings-row">
            <span className="setting-name">MQTT Broker Status</span>
            <span className={`status-pill ${adminStatus?.mqttConfigured ? "online" : "offline"}`}>
              {adminStatus?.mqttConfigured ? "Configured & Subscribed" : "Standalone / Direct"}
            </span>
          </div>
        </Panel>

        {/* Security & Intelligence Posture */}
        <Panel
          icon={ShieldCheck}
          title="Cryptographic & AI Posture"
          subtitle="Encryption guarantees and neural model verification"
        >
          <div className="settings-row">
            <span className="setting-name">RF Packet Verification</span>
            <strong className="setting-val">AES-128 GCM Replay Guard</strong>
          </div>
          <div className="settings-row">
            <span className="setting-name">Transport Encryption</span>
            <strong className="setting-val">TLS 1.3 / HTTPS & WSS</strong>
          </div>
          <div className="settings-row">
            <span className="setting-name">AI Hazard Engine</span>
            <span className="badge tone-green">Explainable Neural v1</span>
          </div>
          <div className="settings-row">
            <span className="setting-name">LLM Reasoning Support</span>
            <span className={`status-pill ${adminStatus?.llmConfigured ? "online" : "offline"}`}>
              {adminStatus?.llmConfigured ? "OpenAI-Compatible RAG Active" : "Local Rule Heuristic"}
            </span>
          </div>
          <div className="settings-row">
            <span className="setting-name">Public User Registration</span>
            <span className={`status-pill ${adminStatus?.registrationEnabled ? "online" : "offline"}`}>
              {adminStatus?.registrationEnabled ? "Enabled (Policy Open)" : "Restricted"}
            </span>
          </div>
        </Panel>
      </div>

      <div className="admin-notes-card">
        <h4>Operator Security Notice</h4>
        <p>
          Sankat-Net operations enforce strict separation between Administrative
          Command and Public Citizen portals. Administrator accounts can enroll field stations,
          dispatch regional alerts, and calibrate threshold metrics.
        </p>
      </div>
    </div>
  );
}
