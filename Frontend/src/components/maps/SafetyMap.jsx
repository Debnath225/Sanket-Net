import React, { useState } from "react";
import { Circle, CircleMarker, MapContainer, Marker, Popup, TileLayer } from "react-leaflet";
import { Home, Info, LifeBuoy, MapPin, Phone, ShieldCheck, Siren } from "lucide-react";
import { formatHazardName } from "../common/Badge.jsx";

// Standard regional relief shelters
const COMMUNITY_SHELTERS = [
  {
    id: "SHELTER-01",
    name: "Central Community Relief Center",
    lat: 22.5750,
    lon: 88.3690,
    capacity: "450 Persons",
    contact: "1077 / 033-2214-5555",
    supplies: "Drinking water, emergency rations, medical triage",
    status: "OPEN & ACTIVE",
  },
  {
    id: "SHELTER-02",
    name: "Higher Secondary Safe Assembly Ground",
    lat: 22.5680,
    lon: 88.3580,
    capacity: "800 Persons",
    contact: "033-2214-3322",
    supplies: "Power backup, dry food packs, blankets",
    status: "OPEN & ACTIVE",
  },
  {
    id: "SHELTER-03",
    name: "District Sports Complex Emergency Camp",
    lat: 22.5810,
    lon: 88.3750,
    capacity: "1,200 Persons",
    contact: "NDRF Camp: 9711077372",
    supplies: "Helipad access, boats, emergency hospital",
    status: "STANDBY READY",
  },
];

export function SafetyMap({ nodes = [], height = "520px" }) {
  const [showShelters, setShowShelters] = useState(true);
  const [showSensors, setShowSensors] = useState(true);
  const [showHazards, setShowHazards] = useState(true);

  const center = [22.5726, 88.3639];
  const validNodes = nodes.filter(
    (n) => Number.isFinite(n.lat) && Number.isFinite(n.lon),
  );

  return (
    <div className="safety-map-wrapper">
      <div className="safety-map-controls">
        <button
          className={`filter-chip ${showShelters ? "active" : ""}`}
          onClick={() => setShowShelters((v) => !v)}
        >
          <Home size={14} />
          <span>Relief Shelters ({COMMUNITY_SHELTERS.length})</span>
        </button>
        <button
          className={`filter-chip ${showSensors ? "active" : ""}`}
          onClick={() => setShowSensors((v) => !v)}
        >
          <ShieldCheck size={14} />
          <span>Field Stations ({validNodes.length})</span>
        </button>
        <button
          className={`filter-chip ${showHazards ? "active" : ""}`}
          onClick={() => setShowHazards((v) => !v)}
        >
          <Siren size={14} />
          <span>Hazard Danger Zones</span>
        </button>
      </div>

      <div className="safety-map-frame" style={{ height }}>
        <MapContainer
          center={center}
          zoom={13}
          scrollWheelZoom
          className="leaflet-map-root"
        >
          <TileLayer
            attribution='&copy; <a href="https://carto.com/">CARTO</a>'
            url="https://{s}.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}{r}.png"
          />

          {/* Shelters */}
          {showShelters &&
            COMMUNITY_SHELTERS.map((shelter) => (
              <CircleMarker
                key={shelter.id}
                center={[shelter.lat, shelter.lon]}
                radius={12}
                pathOptions={{
                  color: "#059669",
                  fillColor: "#10b981",
                  fillOpacity: 0.9,
                  weight: 3,
                }}
              >
                <Popup>
                  <div className="citizen-shelter-popup">
                    <div className="shelter-head">
                      <Home size={16} />
                      <strong>{shelter.name}</strong>
                    </div>
                    <span className="shelter-status-pill">{shelter.status}</span>
                    <div className="shelter-info">
                      <p><strong>Capacity:</strong> {shelter.capacity}</p>
                      <p><strong>Supplies:</strong> {shelter.supplies}</p>
                      <p><strong>Emergency Contact:</strong> {shelter.contact}</p>
                    </div>
                    <a
                      href={`https://www.google.com/maps/dir/?api=1&destination=${shelter.lat},${shelter.lon}`}
                      target="_blank"
                      rel="noopener noreferrer"
                      className="shelter-nav-btn"
                    >
                      Get Safe Walking Directions →
                    </a>
                  </div>
                </Popup>
              </CircleMarker>
            ))}

          {/* Live Sensor Nodes */}
          {showSensors &&
            validNodes.map((node) => {
              const isHigh = Number(node.risk || 0) >= 60;
              const color = isHigh ? "#ef4444" : "#0284c7";

              return (
                <CircleMarker
                  key={node.id}
                  center={[node.lat, node.lon]}
                  radius={9}
                  pathOptions={{
                    color,
                    fillColor: color,
                    fillOpacity: 0.8,
                    weight: 2,
                  }}
                >
                  <Popup>
                    <div className="citizen-sensor-popup">
                      <strong>Station {node.id}</strong>
                      <p>Hazard: {formatHazardName(node.hazard)}</p>
                      <p>Risk Level: {Math.round(node.risk || 0)}%</p>
                      <p>Status: {node.online ? "Reporting Live" : "Offline"}</p>
                    </div>
                  </Popup>
                </CircleMarker>
              );
            })}

          {/* Warning Perimeters */}
          {showHazards && (
            <Circle
              center={[22.5726, 88.3639]}
              radius={800}
              pathOptions={{
                color: "#ef4444",
                fillColor: "#ef4444",
                fillOpacity: 0.18,
                dashArray: "6, 8",
              }}
            >
              <Popup>
                <div className="citizen-hazard-popup">
                  <div className="hazard-popup-head">
                    <Siren size={16} color="#ef4444" />
                    <strong>Low-Lying Waterlogging Zone</strong>
                  </div>
                  <p>Caution advised. Low-clearance vehicles avoid this corridor during rain.</p>
                </div>
              </Popup>
            </Circle>
          )}
        </MapContainer>
      </div>
    </div>
  );
}
