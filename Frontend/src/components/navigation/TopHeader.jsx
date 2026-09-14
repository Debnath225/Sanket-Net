import React from "react";
import { CircleDot, LogOut, Menu, RefreshCw, ShieldAlert, User } from "lucide-react";
import { useAuth } from "../../context/AuthContext.jsx";
import { useLiveData } from "../../context/LiveDataContext.jsx";

export function TopHeader({ onOpenMobile, title, subtitle }) {
  const { user, logout } = useAuth();
  const { connected, refresh } = useLiveData();

  return (
    <header className="admin-topbar">
      <div className="topbar-left">
        <button
          className="topbar-menu-btn"
          onClick={onOpenMobile}
          aria-label="Toggle navigation"
        >
          <Menu size={20} />
        </button>
        <div className="topbar-title-group">
          <span className="topbar-breadcrumb">SANKAT-NET / OPERATIONS</span>
          <h1 className="topbar-heading">{title || "Command Center"}</h1>
        </div>
      </div>

      <div className="topbar-actions">
        <div className={`uplink-badge ${connected ? "connected" : "disconnected"}`}>
          <CircleDot size={13} className="uplink-dot" />
          <span>{connected ? "Mesh Uplink Live" : "Offline"}</span>
        </div>

        <button
          className="topbar-btn"
          onClick={refresh}
          title="Refresh live telemetry stream"
          aria-label="Refresh telemetry"
        >
          <RefreshCw size={16} />
        </button>

        <div className="admin-user-pill">
          <User size={15} />
          <span className="user-label">{user?.username || "Admin"}</span>
          <span className="role-tag">OP-LEVEL 1</span>
        </div>

        <button
          className="topbar-btn logout"
          onClick={logout}
          title="Sign out of Admin Session"
          aria-label="Sign out"
        >
          <LogOut size={16} />
        </button>
      </div>
    </header>
  );
}
