import React, { useEffect, useMemo, useState } from "react";
import { BrowserRouter, Route, Routes } from "react-router-dom";
import { AppShell } from "./components/AppShell.jsx";
import { LoginScreen } from "./components/LoginScreen.jsx";
import { API_URL, WS_URL, emptySnapshot, mergeNodes } from "./data.js";
import {
  OverviewView,
  NodesView,
  NetworkView,
  AlertsView,
  TelemetryView,
  EventsView,
  SettingsView,
  NotFoundView,
} from "./views.jsx";
import "leaflet/dist/leaflet.css";
import "./styles.css";

const SESSION_KEY = "sankat-net-session";

function savedSession() {
  try {
    const session = JSON.parse(sessionStorage.getItem(SESSION_KEY) || "null");
    return session?.token ? session : null;
  } catch {
    return null;
  }
}

export function App() {
  const [session, setSession] = useState(savedSession);
  const [connected, setConnected] = useState(false);
  const [snapshot, setSnapshot] = useState(emptySnapshot);
  const [nodes, setNodes] = useState([]);
  const [logs, setLogs] = useState([]);
  const logout = () => {
    sessionStorage.removeItem(SESSION_KEY);
    setSession(null);
  };
  const authenticate = (nextSession) => {
    sessionStorage.setItem(SESSION_KEY, JSON.stringify(nextSession));
    setSession(nextSession);
  };

  const ingest = (message) => {
    if (message.type === "snapshot") {
      const next = { ...emptySnapshot, ...(message.data || {}) };
      setSnapshot(next);
      setNodes((current) => mergeNodes(current, next.telemetry || []));
    }
    if (message.type === "telemetry") {
      setSnapshot((current) => ({
        ...current,
        telemetry: [message.data, ...current.telemetry].slice(0, 100),
      }));
      setNodes((current) => mergeNodes(current, [message.data]));
    }
    if (["alerts", "heartbeats", "events"].includes(message.type))
      setSnapshot((current) => ({
        ...current,
        [message.type]: [message.data, ...(current[message.type] || [])].slice(0, 100),
      }));
    if (message.type === "events")
      setLogs((current) => [message.data?.message || "System event received", ...current].slice(0, 50));
  };

  useEffect(() => {
    if (!session?.token) return undefined;
    let socket;
    let retry;
    let disposed = false;
    const connect = () => {
      if (disposed) return;
      socket = new WebSocket(`${WS_URL}?token=${encodeURIComponent(session.token)}`);
      socket.onopen = () => setConnected(true);
      socket.onmessage = (event) => {
        try {
          ingest(JSON.parse(event.data));
        } catch {
          setLogs((current) => ["Ignored an invalid live-data message", ...current].slice(0, 50));
        }
      };
      socket.onerror = () => setConnected(false);
      socket.onclose = () => {
        setConnected(false);
        if (!disposed) retry = window.setTimeout(connect, 5000);
      };
    };
    fetch(`${API_URL}/api/snapshot`, {
      headers: { Authorization: `Bearer ${session.token}` },
    })
      .then((response) => {
        if (response.status === 401) {
          logout();
          throw new Error("Session expired");
        }
        if (!response.ok) throw new Error(`Snapshot request failed: ${response.status}`);
        return response.json();
      })
      .then((data) => ingest({ type: "snapshot", data }))
      .catch(() =>
        setLogs((current) => [
          "Backend unavailable; waiting for live data",
          ...current,
        ].slice(0, 50)),
      );
    connect();
    return () => {
      disposed = true;
      window.clearTimeout(retry);
      socket?.close();
    };
  }, [session?.token]);

  const value = useMemo(
    () => ({
      connected,
      nodes,
      telemetry: snapshot.telemetry,
      alerts: snapshot.alerts,
      events: snapshot.events,
      logs,
      refresh: () => window.location.reload(),
      logout,
    }),
    [connected, nodes, snapshot, logs],
  );
  if (!session) return <LoginScreen onAuthenticated={authenticate} />;
  return (
    <BrowserRouter>
      <DataContext.Provider value={value}>
        <Routes>
          <Route
            element={
              <AppShell
                connected={connected}
                onRefresh={value.refresh}
                onLogout={value.logout}
              />
            }
          >
            <Route index element={<OverviewView {...value} />} />
            <Route path="nodes" element={<NodesView {...value} />} />
            <Route path="network" element={<NetworkView {...value} />} />
            <Route
              path="alerts"
              element={<AlertsView alerts={value.alerts} />}
            />
            <Route
              path="telemetry"
              element={
                <TelemetryView
                  telemetry={value.telemetry}
                  nodes={value.nodes}
                />
              }
            />
            <Route path="events" element={<EventsView events={value.events} />} />
            <Route
              path="settings"
              element={<SettingsView connected={connected} />}
            />
            <Route path="*" element={<NotFoundView />} />
          </Route>
        </Routes>
      </DataContext.Provider>
    </BrowserRouter>
  );
}

import { createContext } from "react";
export const DataContext = createContext(null);
