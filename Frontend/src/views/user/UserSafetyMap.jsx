import React from "react";
import { Compass, ExternalLink, Home, MapPin, Navigation, Phone, ShieldCheck } from "lucide-react";
import { useLiveData } from "../../context/LiveDataContext.jsx";
import { SafetyMap } from "../../components/maps/SafetyMap.jsx";

const SHELTER_DIRECTORY = [
  {
    name: "Central Community Relief Camp",
    address: "Municipal High School Ground, Sector 4",
    capacity: "450 Citizens",
    contact: "1077 / 033-2214-5555",
    amenities: ["Clean Water", "Medical First Aid", "Dry Food", "Backup Generator"],
    lat: 22.5750,
    lon: 88.3690,
  },
  {
    name: "Higher Secondary Safe Assembly Ground",
    address: "North Bypass Road, Elevated Ground",
    capacity: "800 Citizens",
    contact: "033-2214-3322",
    amenities: ["Blankets", "Child Care Supplies", "Solar Charging", "Doctor On Duty"],
    lat: 22.5680,
    lon: 88.3580,
  },
  {
    name: "District Sports Complex Emergency Base",
    address: "Stadium Complex, West Gate",
    capacity: "1,200 Citizens",
    contact: "NDRF Camp: 9711077372",
    amenities: ["Rescue Boats", "Helipad Clearance", "Field Hospital", "Canteen"],
    lat: 22.5810,
    lon: 88.3750,
  },
];

export function UserSafetyMap() {
  const { nodes } = useLiveData();

  return (
    <div className="user-map-view">
      <header className="user-page-header">
        <div>
          <span className="user-eyebrow">GEOSPATIAL SAFETY</span>
          <h2>Safe Shelters &amp; Evacuation Corridors</h2>
          <p>
            Explore verified government relief camps, safe assembly points, and
            flood avoidance corridors across your district.
          </p>
        </div>
      </header>

      <div className="map-view-split">
        {/* Interactive Map */}
        <div className="map-column">
          <SafetyMap nodes={nodes} height="560px" />
        </div>

        {/* Shelter List */}
        <div className="shelter-sidebar-column">
          <div className="shelter-list-head">
            <Home size={18} />
            <h3>Designated Relief Shelters</h3>
          </div>

          <div className="shelter-cards-scroll">
            {SHELTER_DIRECTORY.map((s, idx) => (
              <div key={idx} className="shelter-directory-card">
                <div className="card-header">
                  <strong>{s.name}</strong>
                  <span className="capacity-badge">{s.capacity}</span>
                </div>
                <p className="shelter-address">
                  <MapPin size={13} />
                  <span>{s.address}</span>
                </p>
                <div className="amenities-row">
                  {s.amenities.map((a, i) => (
                    <span key={i} className="amenity-pill">
                      {a}
                    </span>
                  ))}
                </div>
                <div className="card-actions">
                  <a
                    href={`tel:${s.contact.split("/")[0].trim()}`}
                    className="shelter-call-btn"
                  >
                    <Phone size={13} />
                    <span>Call Base</span>
                  </a>
                  <a
                    href={`https://www.google.com/maps/dir/?api=1&destination=${s.lat},${s.lon}`}
                    target="_blank"
                    rel="noopener noreferrer"
                    className="shelter-directions-btn"
                  >
                    <Navigation size={13} />
                    <span>Walking Directions</span>
                  </a>
                </div>
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
