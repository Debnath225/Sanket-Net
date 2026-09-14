import React, { useState } from "react";
import { Link, NavLink } from "react-router-dom";
import {
  AlertTriangle,
  BookOpen,
  CloudRain,
  LifeBuoy,
  LogOut,
  MapPin,
  Menu,
  PhoneCall,
  Send,
  ShieldCheck,
  Siren,
  User,
  X,
} from "lucide-react";
import { useAuth } from "../../context/AuthContext.jsx";
import { useLiveData } from "../../context/LiveDataContext.jsx";

const userNavLinks = [
  { path: "/user", label: "Safety Hub", icon: ShieldCheck, end: true },
  { path: "/user/alerts", label: "Alerts & Warnings", icon: Siren },
  { path: "/user/map", label: "Shelters & Safe Map", icon: MapPin },
  { path: "/user/guidance", label: "Go-Bag & Readiness", icon: BookOpen },
  { path: "/user/report", label: "Report Incident", icon: Send },
  { path: "/user/sos", label: "Emergency Directory", icon: PhoneCall },
];

export function UserNavbar() {
  const { user, logout } = useAuth();
  const { alerts } = useLiveData();
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);

  const activeThreats = alerts.filter((a) => Number(a.risk || 0) >= 45).length;

  return (
    <header className="user-navbar-wrapper">
      {activeThreats > 0 && (
        <div className="user-emergency-banner">
          <div className="banner-content">
            <AlertTriangle size={16} />
            <span>
              <strong>Regional Alert Notice:</strong> {activeThreats} active weather/hazard alert(s) in effect. Check advisories below.
            </span>
          </div>
          <Link to="/user/alerts" className="banner-link">
            View Actions →
          </Link>
        </div>
      )}

      <div className="user-navbar">
        <div className="user-nav-left">
          <Link to="/user" className="user-brand">
            <div className="brand-badge-user">
              <ShieldCheck size={20} />
            </div>
            <div className="brand-text-user">
              <span className="brand-title-user">Sankat-Net</span>
              <span className="brand-tag-user">Citizen Safety Portal</span>
            </div>
          </Link>
        </div>

        <nav className="user-desktop-links">
          {userNavLinks.map((item) => {
            const Icon = item.icon;
            return (
              <NavLink
                key={item.path}
                to={item.path}
                end={item.end}
                className={({ isActive }) =>
                  `user-nav-item ${isActive ? "active" : ""}`
                }
              >
                <Icon size={16} />
                <span>{item.label}</span>
              </NavLink>
            );
          })}
        </nav>

        <div className="user-nav-right">
          <Link to="/user/sos" className="sos-quick-btn" title="Emergency Contacts">
            <PhoneCall size={14} />
            <span>SOS 112</span>
          </Link>

          <div className="user-profile-badge">
            <User size={15} />
            <span className="user-name">{user?.username || "Citizen"}</span>
          </div>

          <button
            className="user-logout-btn"
            onClick={logout}
            title="Sign out of Sankat-Net"
            aria-label="Sign out"
          >
            <LogOut size={16} />
          </button>

          <button
            className="user-hamburger"
            onClick={() => setMobileMenuOpen((o) => !o)}
            aria-label="Toggle navigation menu"
          >
            {mobileMenuOpen ? <X size={22} /> : <Menu size={22} />}
          </button>
        </div>
      </div>

      {mobileMenuOpen && (
        <div className="user-mobile-drawer">
          <nav className="user-mobile-nav">
            {userNavLinks.map((item) => {
              const Icon = item.icon;
              return (
                <NavLink
                  key={item.path}
                  to={item.path}
                  end={item.end}
                  onClick={() => setMobileMenuOpen(false)}
                  className={({ isActive }) =>
                    `user-mobile-item ${isActive ? "active" : ""}`
                  }
                >
                  <Icon size={18} />
                  <span>{item.label}</span>
                </NavLink>
              );
            })}
            <div className="user-mobile-actions">
              <Link
                to="/user/sos"
                className="user-mobile-sos"
                onClick={() => setMobileMenuOpen(false)}
              >
                <PhoneCall size={18} />
                <span>National Emergency 112</span>
              </Link>
              <button
                className="user-mobile-logout"
                onClick={() => {
                  setMobileMenuOpen(false);
                  logout();
                }}
              >
                <LogOut size={18} />
                <span>Sign Out ({user?.username})</span>
              </button>
            </div>
          </nav>
        </div>
      )}
    </header>
  );
}
