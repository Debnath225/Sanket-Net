import React from "react";
import {
  AlertTriangle,
  Flame,
  HeartPulse,
  LifeBuoy,
  PhoneCall,
  PhoneForwarded,
  Shield,
  ShieldAlert,
  Zap,
} from "lucide-react";

const EMERGENCY_SERVICES = [
  {
    category: "Primary Emergency Services",
    contacts: [
      { name: "National Unified Emergency Helpline", number: "112", note: "Police, Fire, Medical, Rescue all-in-one", critical: true },
      { name: "State Disaster Management Control Room", number: "1070", note: "Toll-free 24/7 disaster management desk", critical: true },
      { name: "District Emergency Operations Center (DEOC)", number: "1077", note: "Local district collector emergency desk" },
      { name: "Ambulance & Medical Emergency", number: "108", note: "Emergency medical triage and patient transfer" },
      { name: "Fire & Rescue Operations", number: "101", note: "Water rescue, fire fighting, structural collapse" },
      { name: "Police Assistance", number: "100", note: "Law enforcement, evacuation enforcement, safety" },
    ],
  },
  {
    category: "Specialized Crisis Helplines",
    contacts: [
      { name: "National Disaster Response Force (NDRF)", number: "011-24363260", note: "Elite specialized federal rescue battalion" },
      { name: "Women Helpline (Domestic & Disaster Safety)", number: "1091", note: "Emergency safe housing and protection" },
      { name: "Child Emergency Helpline (POCSO/Rescue)", number: "1098", note: "Lost children, orphan emergency shelter" },
      { name: "Railway Emergency & Relief Operations", number: "139", note: "Track waterlogging, passenger safety status" },
    ],
  },
];

const FIRST_AID_TIPS = [
  {
    title: "Hypothermia / Prolonged Water Exposure",
    icon: HeartPulse,
    steps: "Remove wet clothing immediately. Wrap in dry thermal foil blankets or multiple layers. Provide warm, sweet, non-caffeinated drinks if conscious. Do not rub skin vigorously.",
  },
  {
    title: "Electrical Shock & Downed Wires",
    icon: Zap,
    steps: "Never touch the victim directly if still in contact with electrical current. Shut off main supply. Use dry non-conductive wood/plastic to separate. Check breathing and call 112.",
  },
  {
    title: "Waterborne Contamination & Dehydration",
    icon: LifeBuoy,
    steps: "Drink only boiled or chemically treated water. Administer Oral Rehydration Salts (ORS) solution regularly. Seek medical help if severe diarrhea or fever develops.",
  },
];

export function UserEmergencySos() {
  return (
    <div className="user-sos-view">
      <header className="user-page-header">
        <div>
          <span className="user-eyebrow">CRITICAL DIRECTORY</span>
          <h2>Instant Emergency Helplines &amp; SOS Services</h2>
          <p>
            Tap any telephone number to initiate an immediate emergency voice call.
            These verified lines operate 24 hours a day, 7 days a week.
          </p>
        </div>
      </header>

      {/* Services Grid */}
      <div className="emergency-sections-container">
        {EMERGENCY_SERVICES.map((section, idx) => (
          <section key={idx} className="emergency-category-card">
            <h3 className="category-title">{section.category}</h3>
            <div className="contacts-grid">
              {section.contacts.map((contact, cIdx) => (
                <div
                  key={cIdx}
                  className={`contact-card ${contact.critical ? "highlight-critical" : ""}`}
                >
                  <div className="contact-info">
                    <strong className="contact-name">{contact.name}</strong>
                    <p className="contact-note">{contact.note}</p>
                  </div>
                  <a
                    href={`tel:${contact.number.replace(/[^0-9]/g, "")}`}
                    className="contact-dial-btn"
                  >
                    <PhoneCall size={16} />
                    <span>Dial {contact.number}</span>
                  </a>
                </div>
              ))}
            </div>
          </section>
        ))}
      </div>

      {/* Emergency First Aid Cards */}
      <section className="first-aid-section">
        <h3 className="first-aid-heading">
          <HeartPulse size={20} />
          <span>Emergency First Aid Guidance</span>
        </h3>
        <div className="first-aid-grid">
          {FIRST_AID_TIPS.map((tip, i) => {
            const Icon = tip.icon;
            return (
              <div key={i} className="first-aid-card">
                <div className="tip-head">
                  <Icon size={18} />
                  <strong>{tip.title}</strong>
                </div>
                <p className="tip-steps">{tip.steps}</p>
              </div>
            );
          })}
        </div>
      </section>
    </div>
  );
}
