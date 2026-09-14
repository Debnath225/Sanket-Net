import React, { useState } from "react";
import {
  AlertCircle,
  AlertTriangle,
  ArrowRight,
  CheckCircle,
  Clock,
  Droplets,
  ExternalLink,
  Flame,
  Info,
  LifeBuoy,
  MapPin,
  ShieldAlert,
  Siren,
  Wind,
} from "lucide-react";
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { formatHazardName, getRiskLevel } from "../../components/common/Badge.jsx";
import { EmptyState } from "../../components/common/EmptyState.jsx";

const HAZARD_ADVICE = {
  flood: [
    "Move essential documents and medications to upper levels.",
    "Do not drive or walk through moving water; 6 inches can knock you down.",
    "Unplug major electrical appliances if water threatens your home.",
    "Keep battery-powered torches and charged phones within arm's reach.",
  ],
  landslide: [
    "Stay alert to unusual sounds like trees cracking or boulders knocking.",
    "Evacuate immediately if instructed by disaster personnel.",
    "Move away from the path of debris flow to stable high ground.",
  ],
  fire: [
    "Close all doors and windows to reduce smoke intake.",
    "Wear an N95 or damp cloth mask over nose and mouth.",
    "Identify two clear escape routes away from windward smoke direction.",
  ],
  general: [
    "Monitor official Sankat-Net bulletins and local civil radio.",
    "Keep emergency numbers (112, 1070) handy.",
    "Check on elderly neighbors and families with young children.",
  ],
};

export function UserAlerts() {
  const { alerts } = useLiveData();
  const [filter, setFilter] = useState("all");

  const filteredAlerts = alerts.filter((alert) => {
    if (filter === "critical") return Number(alert.risk || 0) >= 75;
    if (filter === "warning") return Number(alert.risk || 0) >= 45 && Number(alert.risk || 0) < 75;
    return true;
  });

  return (
    <div className="user-alerts-view">
      <header className="user-page-header">
        <div>
          <span className="user-eyebrow">COMMUNITY EMERGENCY NOTICES</span>
          <h2>Live Hazard Alerts &amp; Safety Advisories</h2>
          <p>
            Plain-language early warnings automatically triaged by the Sankat-Net
            distributed sensor mesh and AI prediction engine.
          </p>
        </div>

        <div className="alert-filter-tabs">
          <button
            className={`filter-tab ${filter === "all" ? "active" : ""}`}
            onClick={() => setFilter("all")}
          >
            All Notices ({alerts.length})
          </button>
          <button
            className={`filter-tab ${filter === "critical" ? "active" : ""}`}
            onClick={() => setFilter("critical")}
          >
            Critical Only
          </button>
          <button
            className={`filter-tab ${filter === "warning" ? "active" : ""}`}
            onClick={() => setFilter("warning")}
          >
            Advisories
          </button>
        </div>
      </header>

      {filteredAlerts.length > 0 ? (
        <div className="citizen-alerts-feed">
          {filteredAlerts.map((alert, idx) => {
            const riskInfo = getRiskLevel(alert.risk);
            const hazardType = String(alert.hazard || "flood").toLowerCase();
            const adviceList =
              HAZARD_ADVICE[hazardType] || HAZARD_ADVICE.general;

            return (
              <article
                key={`${alert.node_id}-${idx}`}
                className={`citizen-alert-card tone-${riskInfo.tone}`}
              >
                <div className="alert-card-header">
                  <div className="alert-header-left">
                    <div className={`alert-indicator-bubble ${riskInfo.tone}`}>
                      <Siren size={20} />
                    </div>
                    <div>
                      <h3 className="alert-card-title">
                        {formatHazardName(alert.hazard)} Warning — Station {alert.node_id}
                      </h3>
                      <div className="alert-meta-row">
                        <span className="alert-time">
                          <Clock size={13} />
                          {new Date(
                            alert.createdAt || alert.receivedAt || Date.now(),
                          ).toLocaleString()}
                        </span>
                        <span className={`risk-tag tone-${riskInfo.tone}`}>
                          {riskInfo.label} ({Math.round(alert.risk || 0)}%)
                        </span>
                      </div>
                    </div>
                  </div>
                </div>

                <div className="alert-card-body">
                  <div className="alert-instructions-box">
                    <strong>Recommended Immediate Actions:</strong>
                    <ul className="advice-checklist">
                      {adviceList.map((item, i) => (
                        <li key={i}>
                          <CheckCircle size={15} className="check-icon" />
                          <span>{item}</span>
                        </li>
                      ))}
                    </ul>
                  </div>
                </div>

                <footer className="alert-card-footer">
                  <div className="footer-left">
                    <span>Issued via Sankat-Net Neural Predictor</span>
                  </div>
                  <div className="footer-right">
                    <a href="tel:112" className="alert-call-btn">
                      Call 112 Help
                    </a>
                  </div>
                </footer>
              </article>
            );
          })}
        </div>
      ) : (
        <EmptyState
          icon={ShieldAlert}
          title="All Regional Sectors Clear"
          description="There are currently no active emergency alerts in your community. Sensor monitoring remains operational 24/7."
        />
      )}
    </div>
  );
}
