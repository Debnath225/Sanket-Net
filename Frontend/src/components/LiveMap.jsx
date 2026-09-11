import React, { useEffect } from "react";
import { MapPin } from "lucide-react";
import {
  CircleMarker,
  MapContainer,
  Popup,
  TileLayer,
  useMap,
} from "react-leaflet";
import { markerColor } from "./shared.jsx";

function MapViewport({ center, hasNodes }) {
  const map = useMap();

  useEffect(() => {
    map.setView(center, hasNodes ? 12 : 5, { animate: hasNodes });
  }, [center, hasNodes, map]);

  return null;
}

export function LiveMap({ nodes }) {
  const validNodes = nodes.filter(
    (node) => Number.isFinite(node.lat) && Number.isFinite(node.lon),
  );
  const center = validNodes.length
    ? [validNodes[0].lat, validNodes[0].lon]
    : [22.5726, 88.3639];

  return (
    <div className="live-map-wrap">
      <MapContainer
        center={center}
        zoom={validNodes.length ? 12 : 5}
        scrollWheelZoom
        className="live-map"
      >
        <MapViewport center={center} hasNodes={Boolean(validNodes.length)} />
        <TileLayer
          attribution="&copy; OpenStreetMap contributors"
          url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
        />
        {validNodes.map((node) => (
          <CircleMarker
            key={node.id}
            center={[node.lat, node.lon]}
            radius={9}
            pathOptions={{
              color: markerColor(node.risk),
              fillColor: markerColor(node.risk),
              fillOpacity: 0.9,
              weight: 2,
            }}
          >
            <Popup>
              <div className="map-popup">
                <strong>{node.id}</strong>
                <span>{node.role || "Field node"}</span>
                <span>
                  Risk: <b>{Math.round(node.risk || 0)}%</b>
                </span>
                <span>Battery: {node.battery ?? "-"}%</span>
                <span>Status: {node.online ? "Online" : "Offline"}</span>
              </div>
            </Popup>
          </CircleMarker>
        ))}
      </MapContainer>
      {!validNodes.length && (
        <div className="map-empty">
          <MapPin size={18} />
          <span>Waiting for GPS coordinates from field nodes</span>
        </div>
      )}
    </div>
  );
}
