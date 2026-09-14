import React from "react";
import { Link } from "react-router-dom";
import {
  AlertTriangle,
  ArrowRight,
  BookOpen,
  CheckCircle,
  CloudRain,
  Compass,
  Droplets,
  HeartPulse,
  LifeBuoy,
  MapPin,
  PhoneCall,
  Send,
  ShieldAlert,
  ShieldCheck,
  Siren,
} from "lucide-react";
import { useAuth } from "../../context/AuthContext.jsx";
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { formatHazardName, getRiskLevel } from "../../components/common/Badge.jsx";

export function UserHome() {
  const { user } = useAuth();
  const { alerts, nodes, connected } = useLiveData();

  const maxRisk = alerts.length
    ? Math.max(...alerts.map((a) => Number(a.risk || 0)))
    : nodes.length
      ? Math.max(...nodes.map((n) => Number(n.risk || 0)))
      : 12;

  const threatInfo = getRiskLevel(maxRisk);
  const activeAlerts = alerts.filter((a) => Number(a.risk || 0) >= 40);

  return (
    <div className="user-home-view">
      {/* Friendly Welcome & Threat Gauge */}
      <section className={`user-hero-card threat-${threatInfo.tone}`}>
        <div className="hero-content">
          <div className="threat-pill">
            <span className="dot" />
            <span>CURRENT REGIONAL THREAT STATUS</span>
          </div>

          <h2 className="user-hero-title">
            {maxRisk >= 75 ? (
              <>Severe Incident Warning Active in Sector</>
            ) : maxRisk >= 45 ? (
              <>Precautionary Flood &amp; Rain Advisory</>
            ) : (
              <>All Regional Sectors Reporting Normal</>
            )}
          </h2>

          <p className="user-hero-copy">
            {maxRisk >= 75
              ? "High environmental thresholds detected. Keep emergency go-bag ready and monitor local shelter instructions."
              : maxRisk >= 45
                ? "Moderate localized water accumulation or rainfall detected. Check safe travel routes before commuting."
                : "Continuous monitoring active across field sensor mesh. No immediate disaster warnings for your location."}
          </p>

          <div className="user-quick-actions">
            <Link to="/user/alerts" className="user-primary-btn">
              <Siren size={16} />
              <span>Review Active Alerts ({alerts.length})</span>
            </Link>
            <Link to="/user/map" className="user-secondary-btn">
              <MapPin size={16} />
              <span>Find Safe Shelters</span>
            </Link>
          </div>
        </div>

        <div className="threat-gauge-visual">
          <div className="gauge-circle">
            <span className="gauge-score">{Math.round(maxRisk)}%</span>
            <span className="gauge-label">{threatInfo.label}</span>
          </div>
          <small className="gauge-sub">AI Hyperlocal Index</small>
        </div>
      </section>

      {/* 1-Tap Emergency SOS Bar */}
      <section className="emergency-call-bar">
        <div className="sos-bar-title">
          <PhoneCall size={18} />
          <strong>Instant Emergency Helplines</strong>
        </div>
        <div className="sos-chips-row">
          <a href="tel:112" className="sos-phone-chip critical">
            <span>National Emergency</span>
            <strong>112</strong>
          </a>
          <a href="tel:1070" className="sos-phone-chip">
            <span>Disaster Management</span>
            <strong>1070</strong>
          </a>
          <a href="tel:108" className="sos-phone-chip">
            <span>Ambulance / Medical</span>
            <strong>108</strong>
          </a>
          <a href="tel:101" className="sos-phone-chip">
            <span>Fire &amp; Rescue</span>
            <strong>101</strong>
          </a>
        </div>
      </section>

      {/* Feature Grid */}
      <div className="user-features-grid">
        {/* Active Alerts Teaser */}
        <div className="user-feature-card">
          <div className="card-top">
            <div className="card-icon-bubble amber">
              <Siren size={20} />
            </div>
            <span className="card-counter">{activeAlerts.length} Warnings</span>
          </div>
          <h3>Regional Incident Advisories</h3>
          <p>
            Stay informed with verified, plain-language warnings for flash floods,
            landslides, and road blockages.
          </p>
          <div className="card-preview-list">
            {activeAlerts.slice(0, 2).map((alert, i) => (
              <div key={i} className="preview-alert-item">
                <span className="item-title">{formatHazardName(alert.hazard)}</span>
                <span className="item-meta">Station {alert.node_id} · {alert.risk}% Risk</span>
              </div>
            ))}
            {activeAlerts.length === 0 && (
              <p className="empty-preview">No critical advisories in your sector right now.</p>
            )}
          </div>
          <Link to="/user/alerts" className="card-cta">
            <span>View Full Alert Feed</span>
            <ArrowRight size={15} />
          </Link>
        </div>

        {/* Safe Shelters & Map */}
        <div className="user-feature-card">
          <div className="card-top">
            <div className="card-icon-bubble green">
              <MapPin size={20} />
            </div>
            <span className="card-counter">3 Verified Shelters</span>
          </div>
          <h3>Evacuation &amp; Safe Havens</h3>
          <p>
            Locate high-ground relief centers, medical camps, and safe walking corridors
            in case of rising water levels.
          </p>
          <div className="shelter-preview-box">
            <strong>Central Community Relief Camp</strong>
            <small>450 Capacity · Drinking Water &amp; Medical Triage</small>
          </div>
          <Link to="/user/map" className="card-cta">
            <span>Open Interactive Map</span>
            <ArrowRight size={15} />
          </Link>
        </div>

        {/* Go-Bag Readiness Guide */}
        <div className="user-feature-card">
          <div className="card-top">
            <div className="card-icon-bubble cyan">
              <BookOpen size={20} />
            </div>
            <span className="card-counter">Interactive Checklist</span>
          </div>
          <h3>Emergency Go-Bag &amp; Guides</h3>
          <p>
            Build your survival kit with essential medicines, dry rations, battery banks,
            and document protectors before disaster strikes.
          </p>
          <div className="readiness-progress-preview">
            <span>Preparedness Checklist Saved on Device</span>
          </div>
          <Link to="/user/guidance" className="card-cta">
            <span>Open Go-Bag Checklist</span>
            <ArrowRight size={15} />
          </Link>
        </div>

        {/* Report Incident */}
        <div className="user-feature-card">
          <div className="card-top">
            <div className="card-icon-bubble purple">
              <Send size={20} />
            </div>
            <span className="card-counter">Community Help</span>
          </div>
          <h3>Report Local Hazard / SOS</h3>
          <p>
            Report flooded roads, blocked culverts, or stranded citizens to the
            emergency operations network.
          </p>
          <div className="report-note-preview">
            <CheckCircle size={14} />
            <span>Community reports reach local response coordinators</span>
          </div>
          <Link to="/user/report" className="card-cta">
            <span>File a Hazard Report</span>
            <ArrowRight size={15} />
          </Link>
        </div>
      </div>
    </div>
  );
}
