import React, { useState } from "react";
import { Outlet, useLocation } from "react-router-dom";
import { AdminSidebar } from "../components/navigation/AdminSidebar.jsx";
import { TopHeader } from "../components/navigation/TopHeader.jsx";
import { ChatAssistant } from "../components/chat/ChatAssistant.jsx";

const routeTitles = {
  "/admin": { title: "Command Center Overview", subtitle: "Real-time Field Telemetry & AI Hazard Prediction" },
  "/admin/nodes": { title: "Field Monitoring Stations", subtitle: "LoRa Sensor Node Registry & Hardware Diagnostics" },
  "/admin/network": { title: "LoRa Mesh Network Topology", subtitle: "Mesh Routing Matrix, Packet Stream & Gateway Uplink" },
  "/admin/alerts": { title: "Hazard Incidents & Triage", subtitle: "Real-time AI Incident Prioritization & Broadcast" },
  "/admin/telemetry": { title: "Environmental Telemetry Analytics", subtitle: "Multi-parameter Time Series & Sensor Observation" },
  "/admin/events": { title: "Operational System Events", subtitle: "Audit Logs, Gateway Health & MQTT Packet Flow" },
  "/admin/users": { title: "User & Role Management", subtitle: "System Operators, Citizen Roles & Access Control" },
  "/admin/settings": { title: "Diagnostics & Security Settings", subtitle: "Backend Connection, Encryption & LLM Posture" },
};

export function AdminLayout() {
  const [mobileOpen, setMobileOpen] = useState(false);
  const location = useLocation();

  const currentMeta = routeTitles[location.pathname] || {
    title: "Operations Management",
    subtitle: "Sankat-Net Operational Portal",
  };

  return (
    <div className="admin-layout-shell">
      <AdminSidebar mobileOpen={mobileOpen} onClose={() => setMobileOpen(false)} />

      <div className="admin-main-container">
        <TopHeader
          onOpenMobile={() => setMobileOpen(true)}
          title={currentMeta.title}
          subtitle={currentMeta.subtitle}
        />

        <main className="admin-viewport">
          <Outlet />
        </main>
      </div>

      <ChatAssistant />
    </div>
  );
}
