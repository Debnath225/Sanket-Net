import { API_URL, request } from "./client.js";

export const SESSION_KEY = "sankat-net-session";

export function loadSession() {
  try {
    const raw = sessionStorage.getItem(SESSION_KEY);
    if (!raw) return null;
    const session = JSON.parse(raw);
    return session?.token ? session : null;
  } catch {
    return null;
  }
}

export function saveSession(session) {
  if (!session) {
    sessionStorage.removeItem(SESSION_KEY);
  } else {
    sessionStorage.setItem(SESSION_KEY, JSON.stringify(session));
  }
}

export async function login({ username, password, accountType }) {
  const response = await fetch(`${API_URL}/api/auth/login`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ username, password, accountType }),
  });

  const data = await response.json().catch(() => ({}));
  if (!response.ok || !data.token) {
    const error = new Error(data?.error || "login_failed");
    error.status = response.status;
    error.data = data;
    throw error;
  }
  return data;
}

export async function register({ username, password, accountType = "OPERATOR", adminCode }) {
  const response = await fetch(`${API_URL}/api/auth/register`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      username,
      password,
      accountType,
      ...(accountType === "ADMIN" ? { adminCode } : {}),
    }),
  });

  const data = await response.json().catch(() => ({}));
  if (!response.ok || !data.token) {
    const error = new Error(data?.error || "registration_failed");
    error.status = response.status;
    error.data = data;
    throw error;
  }
  return data;
}
