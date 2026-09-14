import React, { useEffect, useState } from "react";
import {
  AlertTriangle,
  BookOpen,
  CheckCircle2,
  Circle,
  CloudRain,
  Compass,
  Droplets,
  FileText,
  HeartPulse,
  PackageCheck,
  RotateCcw,
  ShieldCheck,
  Zap,
} from "lucide-react";

const GO_BAG_ITEMS = [
  { id: "water", title: "Clean Water", desc: "At least 3 liters of sealed drinking water per person" },
  { id: "rations", title: "Non-Perishable Food", desc: "Energy bars, dry fruits, biscuits, canned goods (3-day supply)" },
  { id: "torch", title: "Flashlight / Torch", desc: "High-lumen LED torch with spare batteries" },
  { id: "firstaid", title: "First Aid & Medicine", desc: "Bandages, antiseptic, ORS, pain relievers & personal prescriptions" },
  { id: "powerbank", title: "Power Bank & Cables", desc: "Fully charged high-capacity power bank for mobile phone" },
  { id: "documents", title: "Vital Documents in Waterproof Ziploc", desc: "IDs, insurance cards, land deeds, bank records, and cash" },
  { id: "whistle", title: "Emergency Whistle", desc: "Loud whistle to signal location to search & rescue teams" },
  { id: "mask", title: "Dust Mask / N95", desc: "Protects airway from dust, smoke, and airborne debris" },
  { id: "blanket", title: "Emergency Thermal Foil Blanket", desc: "Prevents hypothermia if caught in wet, cold environments" },
  { id: "clothes", title: "Spare Change of Clothes & Rain Poncho", desc: "Sturdy boots, warm socks, and waterproof outer layer" },
];

const GUIDE_TOPICS = [
  {
    id: "flood",
    title: "Flood & Flash Floods",
    icon: Droplets,
    content: [
      "If warned of flash flooding, move immediately to higher ground. Do not wait for instructions.",
      "Never attempt to drive or walk through flood waters. Six inches of rushing water can knock down an adult.",
      "Stay off bridges over fast-moving water as they may be washed away without warning.",
      "Switch off main electricity fuses and gas valves before evacuating your home.",
    ],
  },
  {
    id: "landslide",
    title: "Landslide & Mudflow",
    icon: AlertTriangle,
    content: [
      "Listen for rumbling sounds that gradually increase in volume, or trees cracking.",
      "Watch for new cracks in plaster, tile, brick, or foundations, and leaning fence posts.",
      "If trapped, curl into a tight ball and protect your head to survive debris impact.",
      "Stay away from the slide area. There is high danger of additional secondary slides.",
    ],
  },
  {
    id: "storm",
    title: "Cyclones & High Winds",
    icon: CloudRain,
    content: [
      "Board up or shutter all windows. Draw blinds and curtains to prevent flying glass.",
      "Anchor or bring indoors all loose objects such as lawn chairs, garbage cans, and tools.",
      "Remain indoors in an interior windowless room or hallway until the all-clear is issued.",
      "Beware of the calm 'eye' of the cyclone; ferocious winds will resume from the opposite direction.",
    ],
  },
  {
    id: "recovery",
    title: "Water Safety & Sanitization",
    icon: ShieldCheck,
    content: [
      "Assume tap water is contaminated until health authorities confirm it is safe to drink.",
      "Boil water vigorously for at least one full minute before drinking or cooking.",
      "Discard any food that has come into contact with floodwater.",
      "Wear thick rubber boots and heavy gloves while cleaning up mud and standing water.",
    ],
  },
];

export function UserPreparedness() {
  const [checkedItems, setCheckedItems] = useState(() => {
    try {
      const saved = localStorage.getItem("sankat_gobag_items");
      return saved ? JSON.parse(saved) : {};
    } catch {
      return {};
    }
  });

  const [activeGuide, setActiveGuide] = useState("flood");

  useEffect(() => {
    try {
      localStorage.setItem("sankat_gobag_items", JSON.stringify(checkedItems));
    } catch {}
  }, [checkedItems]);

  const toggleItem = (id) => {
    setCheckedItems((prev) => ({
      ...prev,
      [id]: !prev[id],
    }));
  };

  const resetChecklist = () => {
    if (window.confirm("Reset all checklist items?")) {
      setCheckedItems({});
    }
  };

  const completedCount = Object.values(checkedItems).filter(Boolean).length;
  const progressPercent = Math.round((completedCount / GO_BAG_ITEMS.length) * 100);

  const selectedGuide = GUIDE_TOPICS.find((g) => g.id === activeGuide) || GUIDE_TOPICS[0];

  return (
    <div className="user-preparedness-view">
      <header className="user-page-header">
        <div>
          <span className="user-eyebrow">DISASTER RESILIENCE MANUAL</span>
          <h2>Emergency Preparedness &amp; Go-Bag Checklist</h2>
          <p>
            Proactive readiness protects lives. Prepare your survival supplies
            and study critical emergency procedures before extreme events occur.
          </p>
        </div>
      </header>

      <div className="preparedness-grid">
        {/* Interactive Go-Bag Checklist */}
        <div className="gobag-section-card">
          <div className="gobag-header">
            <div className="gobag-title-group">
              <PackageCheck size={22} className="gobag-icon" />
              <div>
                <h3>72-Hour Emergency Go-Bag</h3>
                <p>Essential survival supplies for evacuation</p>
              </div>
            </div>
            <button className="reset-btn" onClick={resetChecklist} title="Clear checklist">
              <RotateCcw size={14} />
              <span>Reset</span>
            </button>
          </div>

          <div className="gobag-progress-bar-container">
            <div className="progress-meta">
              <span>
                <strong>{completedCount} of {GO_BAG_ITEMS.length} items packed</strong>
              </span>
              <span className="progress-percent">{progressPercent}% Ready</span>
            </div>
            <div className="progress-track">
              <div
                className="progress-fill"
                style={{ width: `${progressPercent}%` }}
              />
            </div>
          </div>

          <div className="gobag-items-list">
            {GO_BAG_ITEMS.map((item) => {
              const isChecked = Boolean(checkedItems[item.id]);
              return (
                <div
                  key={item.id}
                  className={`gobag-item ${isChecked ? "checked" : ""}`}
                  onClick={() => toggleItem(item.id)}
                  role="checkbox"
                  aria-checked={isChecked}
                  tabIndex={0}
                  onKeyDown={(e) => {
                    if (e.key === " " || e.key === "Enter") {
                      e.preventDefault();
                      toggleItem(item.id);
                    }
                  }}
                >
                  <div className="item-checkbox">
                    {isChecked ? (
                      <CheckCircle2 size={19} className="check-done" />
                    ) : (
                      <Circle size={19} className="check-undone" />
                    )}
                  </div>
                  <div className="item-content">
                    <strong className="item-title">{item.title}</strong>
                    <p className="item-desc">{item.desc}</p>
                  </div>
                </div>
              );
            })}
          </div>
        </div>

        {/* Hazard Protocol Guides */}
        <div className="protocols-section-card">
          <div className="protocols-header">
            <BookOpen size={20} />
            <div>
              <h3>Disaster Survival Protocols</h3>
              <p>Step-by-step guidance for emergency situations</p>
            </div>
          </div>

          <div className="guide-tabs">
            {GUIDE_TOPICS.map((topic) => {
              const Icon = topic.icon;
              return (
                <button
                  key={topic.id}
                  className={`guide-tab-btn ${activeGuide === topic.id ? "active" : ""}`}
                  onClick={() => setActiveGuide(topic.id)}
                >
                  <Icon size={16} />
                  <span>{topic.title}</span>
                </button>
              );
            })}
          </div>

          <div className="guide-content-panel">
            <h4>Action Protocols: {selectedGuide.title}</h4>
            <div className="steps-list">
              {selectedGuide.content.map((step, index) => (
                <div key={index} className="step-row">
                  <span className="step-badge">{index + 1}</span>
                  <p>{step}</p>
                </div>
              ))}
            </div>

            <div className="important-box">
              <ShieldCheck size={18} />
              <p>
                During an actual evacuation order issued by NDMA / Civil Authorities,
                always prioritize saving human lives over property. Leave immediately via marked safe routes.
              </p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
