import React, { createContext, useContext, useEffect, useMemo, useState } from "react";
import { API_URL, WS_URL } from "../api/client.js";
import { useAuth } from "./AuthContext.jsx";

const LiveDataContext = createContext(null);

export const emptySnapshot = {
  telemetry: [],
  alerts: [],
  heartbeats: [],
  events: [],
};

export function mergeNodes(current, records) {
  const merged = new Map(current.map((node) => [node.id, node]));
  (records || []).forEach((record) => {
    if (!record) return;
    const id = String(record.node_id || record.source_node || "UNKNOWN");
    const previous = merged.get(id) || {
      id,
      role: "Field Sensor Station",
      battery: null,
      lat: null,
      lon: null,
    };
    merged.set(id, {
      ...previous,
      online: true,
      risk: Number(record.prediction?.risk ?? record.risk_score ?? previous.risk ?? 0),
      hazard: String(
        record.prediction?.hazard ||
          record.hazard ||
          previous.hazard ||
          "NORMAL",
      ).toUpperCase(),
      lat: Number(record.latitude ?? previous.lat),
      lon: Number(record.longitude ?? previous.lon),
      battery: record.battery_mv
        ? Math.min(100, Math.round(Number(record.battery_mv) / 42))
        : previous.battery,
      temperature: record.temperature ?? previous.temperature ?? null,
      humidity: record.humidity ?? previous.humidity ?? null,
      water_level: record.water_level ?? previous.water_level ?? null,
      soil_moisture: record.soil_moisture ?? previous.soil_moisture ?? null,
      rainfall_mm: record.rainfall_mm ?? previous.rainfall_mm ?? null,
      lastSeen: record.receivedAt || new Date().toISOString(),
    });
  });
  return [...merged.values()];
}

export function LiveDataProvider({ children }) {
  const { session, logout } = useAuth();
  const [connected, setConnected] = useState(false);
  const [snapshot, setSnapshot] = useState(emptySnapshot);
  const [nodes, setNodes] = useState([]);
  const [logs, setLogs] = useState([]);

  const ingest = (message) => {
    if (!message || typeof message !== "object") return;

    if (message.type === "snapshot" && message.data) {
      const next = { ...emptySnapshot, ...message.data };
      setSnapshot(next);
      setNodes((current) => mergeNodes(current, next.telemetry || []));
    }

    if (message.type === "telemetry" && message.data) {
      setSnapshot((current) => ({
        ...current,
        telemetry: [message.data, ...(current.telemetry || [])].slice(0, 150),
      }));
      setNodes((current) => mergeNodes(current, [message.data]));
    }

    if (["alerts", "heartbeats", "events"].includes(message.type) && message.data) {
      setSnapshot((current) => ({
        ...current,
        [message.type]: [message.data, ...(current[message.type] || [])].slice(0, 100),
      }));
    }

    if (message.type === "events" && message.data) {
      const text = message.data.message || "Operational event recorded";
      setLogs((current) => [text, ...current].slice(0, 80));
    }
  };

  const fetchSnapshot = async () => {
    if (!session?.token) return;
    try {
      const response = await fetch(`${API_URL}/api/snapshot`, {
        headers: { Authorization: `Bearer ${session.token}` },
      });
      if (response.status === 401) {
        logout();
        return;
      }
      if (!response.ok) return;
      const data = await response.json();
      ingest({ type: "snapshot", data });
    } catch {
      setLogs((current) => [
        "Backend connecting; waiting for live data stream",
        ...current,
      ].slice(0, 50));
    }
  };

  useEffect(() => {
    if (!session?.token) return undefined;

    let socket = null;
    let retryTimeout = null;
    let disposed = false;

    const connectWs = () => {
      if (disposed) return;
      try {
        socket = new WebSocket(
          `${WS_URL}?token=${encodeURIComponent(session.token)}`,
        );

        socket.onopen = () => {
          if (!disposed) setConnected(true);
        };

        socket.onmessage = (event) => {
          try {
            ingest(JSON.parse(event.data));
          } catch {
            setLogs((current) => [
              "Ignored malformed telemetry stream message",
              ...current,
            ].slice(0, 50));
          }
        };

        socket.onerror = () => {
          if (!disposed) setConnected(false);
        };

        socket.onclose = () => {
          if (!disposed) {
            setConnected(false);
            retryTimeout = window.setTimeout(connectWs, 4000);
          }
        };
      } catch {
        if (!disposed) {
          setConnected(false);
          retryTimeout = window.setTimeout(connectWs, 5000);
        }
      }
    };

    fetchSnapshot();
    connectWs();

    return () => {
      disposed = true;
      window.clearTimeout(retryTimeout);
      socket?.close();
    };
  }, [session?.token]);

  const value = useMemo(
    () => ({
      connected,
      nodes,
      telemetry: snapshot.telemetry || [],
      alerts: snapshot.alerts || [],
      events: snapshot.events || [],
      heartbeats: snapshot.heartbeats || [],
      logs,
      refresh: fetchSnapshot,
    }),
    [connected, nodes, snapshot, logs],
  );

  return (
    <LiveDataContext.Provider value={value}>
      {children}
    </LiveDataContext.Provider>
  );
}

export function useLiveData() {
  const context = useContext(LiveDataContext);
  if (!context) {
    throw new Error("useLiveData must be used within a LiveDataProvider");
  }
  return context;
}
