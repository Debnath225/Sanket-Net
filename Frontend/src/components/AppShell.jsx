import React, { useState } from "react";
import {
  CircleDot,
  ChevronRight,
  Menu,
  RefreshCw,
  LogOut,
  Wifi,
  X,
} from "lucide-react";
import { NavLink, Outlet, useLocation } from "react-router-dom";
import { icons } from "./shared.jsx";
import { navigation } from "../data.js";

export function AppShell({ connected, onRefresh, onLogout }) {
  const [mobileOpen, setMobileOpen] = useState(false);
  const location = useLocation();
  const current =
    navigation.find(([path]) => path === location.pathname) || navigation[0];
  return (
    <div className="app-shell">
      <aside
        id="primary-navigation"
        className={`sidebar ${mobileOpen ? "open" : ""}`}
        aria-label="Primary navigation"
      >
        <div className="brand">
          <div className="brand-mark">SN</div>
          <div>
            <strong>Sankat-Net</strong>
            <span>Environmental intelligence</span>
          </div>
          <button
            className="icon-button close-nav"
            onClick={() => setMobileOpen(false)}
            aria-label="Close navigation"
          >
            <X size={18} />
          </button>
        </div>
        <nav>
          {navigation.map(([path, label, iconName]) => {
            const Icon = icons[iconName];
            return (
              <NavLink
                key={path}
                to={path}
                end={path === "/"}
                onClick={() => setMobileOpen(false)}
                className={({ isActive }) => (isActive ? "active" : "")}
              >
                <Icon size={17} />
                {label}
                <ChevronRight className="nav-arrow" size={15} />
              </NavLink>
            );
          })}
        </nav>
        <div className="sidebar-bottom">
          <div className="gateway-card">
            <div className="gateway-heading">
              <span className={`status-dot ${connected ? "good" : "warn"}`} />{" "}
              Gateway GW-FFFF
            </div>
            <p>{connected ? "MQTT bridge connected" : "Waiting for backend"}</p>
            <div className="gateway-meta">
              <Wifi size={13} /> LoRa mesh · TLS uplink
            </div>
          </div>
          <small>
            AI-assisted hazard detection
            <br />
            Store-and-forward resilient network
          </small>
        </div>
      </aside>
      {mobileOpen && (
        <button
          className="nav-backdrop"
          onClick={() => setMobileOpen(false)}
          aria-label="Close navigation"
        />
      )}
      <main className="main-shell">
        <header className="topbar">
          <button
            className="icon-button mobile-only"
            onClick={() => setMobileOpen(true)}
            aria-label="Open navigation"
            aria-controls="primary-navigation"
            aria-expanded={mobileOpen}
          >
            <Menu size={19} />
          </button>
          <div>
            <p className="eyebrow">SANKAT-NET / SIH26178</p>
            <h1>{current[1]}</h1>
          </div>
          <div className="topbar-actions">
            <span className={`live-chip ${connected ? "connected" : ""}`}>
              <CircleDot size={13} />
              {connected ? "Backend live" : "Offline"}
            </span>
            <button
              className="icon-button"
              onClick={onRefresh}
              aria-label="Refresh dashboard"
            >
              <RefreshCw size={17} />
            </button>
            <button className="icon-button" onClick={onLogout} aria-label="Sign out">
              <LogOut size={17} />
            </button>
          </div>
        </header>
        <div className="page-content">
          <Outlet />
        </div>
      </main>
    </div>
  );
}
