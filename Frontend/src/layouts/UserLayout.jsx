import React from "react";
import { Outlet } from "react-router-dom";
import { UserNavbar } from "../components/navigation/UserNavbar.jsx";
import { ChatAssistant } from "../components/chat/ChatAssistant.jsx";

export function UserLayout() {
  return (
    <div className="user-layout-shell">
      <UserNavbar />

      <main className="user-viewport">
        <div className="user-page-container">
          <Outlet />
        </div>
      </main>

      <footer className="user-footer">
        <div className="user-footer-content">
          <div>
            <strong>Sankat-Net Citizen Safety Network</strong>
            <p>Community Resilience & Hyperlocal Disaster Early Warning System</p>
          </div>
          <div className="user-footer-helplines">
            <span>National Emergency: <strong>112</strong></span>
            <span>Disaster Management: <strong>1070 / 1077</strong></span>
            <span>Ambulance: <strong>108</strong></span>
          </div>
        </div>
      </footer>

      <ChatAssistant />
    </div>
  );
}
