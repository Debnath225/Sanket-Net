import React, { useEffect } from "react";
import { CircleMarker, MapContainer, Popup, TileLayer, useMap } from "react-leaflet";
import { MapPin, Radio, ShieldAlert } from "lucide-react";
import { formatHazardName, getRiskLevel } from "../common/Badge.jsx";

function MapViewport({ center, zoom }) {
  const map = useMap();
  useEffect(() => {
    if (center && Number.isFinite(center[0]) && Number.isFinite(center[1])) {
      map.setView(center, zoom, { animate: true });
    }
  }, [center, zoom, map]);
  return null;
}

export function LiveMap({ nodes = [], height = "400px" }) {
  const validNodes = nodes.filter(
    (n) => Number.isFinite(n.lat) && Number.isFinite(n.lon),
  );

  const defaultCenter = [22.5726, 88.3639];
  const center = validNodes.length ? [validNodes[0].lat, validNodes[0].lon] : defaultCenter;
  const zoom = validNodes.length ? 12 : 6;

  const getMarkerColor = (risk) => {
    const num = Number(risk || 0);
    if (num >= 75) return "#ef4444";
    if (num >= 45) return "#f59e0b";
    if (num >= 20) return "#eab308";
    return "#10b981";
  };

  return (
    <div className="live-map-container" style={{ height }}>
      <MapContainer
        center={center}
        zoom={zoom}
        scrollWheelZoom
        className="leaflet-map-root"
      >
        <MapViewport center={center} zoom={zoom} />
        <TileLayer
          attribution='&copy; <a href="https://carto.com/">CARTO</a>'
          url="https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png"
        />

        {validNodes.map((node) => {
          const color = getMarkerColor(node.risk);
          const riskInfo = getRiskLevel(node.risk);

          return (
            <CircleMarker
              key={node.id}
              center={[node.lat, node.lon]}
              radius={10}
              pathOptions={{
                color,
                fillColor: color,
                fillOpacity: 0.85,
                weight: 2,
              }}
            >
              <Popup>
                <div className="admin-map-popup">
                  <div className="popup-head">
                    <Radio size={14} />
                    <strong>{node.id}</strong>
                    <span className={`popup-status ${node.online ? "online" : "offline"}`}>
                      {node.online ? "ONLINE" : "OFFLINE"}
                    </span>
                  </div>
                  <div className="popup-body">
                    <div className="popup-row">
                      <span>Hazard Risk:</span>
                      <strong style={{ color }}>
                        {formatHazardName(node.hazard)} ({Math.round(node.risk || 0)}%)
                      </strong>
                    </div>
                    <div className="popup-row">
                      <span>Coordinates:</span>
                      <code>{node.lat?.toFixed(4)}, {node.lon?.toFixed(4)}</code>
                    </div>
                    {node.battery !== null && (
                      <div className="popup-row">
                        <span>Battery:</span>
                        <span>{node.battery}%</span>
                      </div>
                    )}
                    {node.water_level !== null && (
                      <div className="popup-row">
                        <span>Water Level:</span>
                        <span>{node.water_level} cm</span>
                      </div>
                    )}
                    {node.soil_moisture !== null && (
                      <div className="popup-row">
                        <span>Soil Moisture:</span>
                        <span>{node.soil_moisture}%</span>
                      </div>
                    )}
                  </div>
                </div>
              </Popup>
            </CircleMarker>
          );
        })}
      </MapContainer>

      {!validNodes.length && (
        <div className="map-overlay-empty">
          <MapPin size={22} />
          <span>Awaiting node GPS telemetry stream...</span>
        </div>
      )}
    </div>
  );
}
