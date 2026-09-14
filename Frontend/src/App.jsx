import React from "react";
import { BrowserRouter, Navigate, Route, Routes } from "react-router-dom";
import { AuthProvider, useAuth } from "./context/AuthContext.jsx";
import { LiveDataProvider } from "./context/LiveDataContext.jsx";
import { AuthScreen } from "./views/auth/AuthScreen.jsx";
import { AdminLayout } from "./layouts/AdminLayout.jsx";
import { UserLayout } from "./layouts/UserLayout.jsx";

// Admin Views
import { AdminOverview } from "./views/admin/AdminOverview.jsx";
import { AdminNodes } from "./views/admin/AdminNodes.jsx";
import { AdminNetwork } from "./views/admin/AdminNetwork.jsx";
import { AdminAlerts } from "./views/admin/AdminAlerts.jsx";
import { AdminTelemetry } from "./views/admin/AdminTelemetry.jsx";
import { AdminEvents } from "./views/admin/AdminEvents.jsx";
import { AdminUsers } from "./views/admin/AdminUsers.jsx";
import { AdminSettings } from "./views/admin/AdminSettings.jsx";

// User Views
import { UserHome } from "./views/user/UserHome.jsx";
import { UserAlerts } from "./views/user/UserAlerts.jsx";
import { UserSafetyMap } from "./views/user/UserSafetyMap.jsx";
import { UserPreparedness } from "./views/user/UserPreparedness.jsx";
import { UserHazardReport } from "./views/user/UserHazardReport.jsx";
import { UserEmergencySos } from "./views/user/UserEmergencySos.jsx";

import "leaflet/dist/leaflet.css";
import "./styles.css";

function AppRoutes() {
  const { isAuthenticated, isAdmin } = useAuth();

  if (!isAuthenticated) {
    return <AuthScreen />;
  }

  return (
    <LiveDataProvider>
      <Routes>
        {/* Admin Routes */}
        {isAdmin ? (
          <>
            <Route path="/admin" element={<AdminLayout />}>
              <Route index element={<AdminOverview />} />
              <Route path="nodes" element={<AdminNodes />} />
              <Route path="network" element={<AdminNetwork />} />
              <Route path="alerts" element={<AdminAlerts />} />
              <Route path="telemetry" element={<AdminTelemetry />} />
              <Route path="events" element={<AdminEvents />} />
              <Route path="users" element={<AdminUsers />} />
              <Route path="settings" element={<AdminSettings />} />
              <Route path="*" element={<Navigate to="/admin" replace />} />
            </Route>
            <Route path="/" element={<Navigate to="/admin" replace />} />
            <Route path="/user/*" element={<Navigate to="/admin" replace />} />
          </>
        ) : (
          /* Citizen / User Routes */
          <>
            <Route path="/user" element={<UserLayout />}>
              <Route index element={<UserHome />} />
              <Route path="alerts" element={<UserAlerts />} />
              <Route path="map" element={<UserSafetyMap />} />
              <Route path="guidance" element={<UserPreparedness />} />
              <Route path="report" element={<UserHazardReport />} />
              <Route path="sos" element={<UserEmergencySos />} />
              <Route path="*" element={<Navigate to="/user" replace />} />
            </Route>
            <Route path="/" element={<Navigate to="/user" replace />} />
            <Route path="/admin/*" element={<Navigate to="/user" replace />} />
          </>
        )}
        <Route path="*" element={<Navigate to={isAdmin ? "/admin" : "/user"} replace />} />
      </Routes>
    </LiveDataProvider>
  );
}

export function App() {
  return (
    <BrowserRouter>
      <AuthProvider>
        <AppRoutes />
      </AuthProvider>
    </BrowserRouter>
  );
}
