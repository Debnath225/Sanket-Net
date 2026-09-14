export const API_URL =
  import.meta.env.VITE_BACKEND_URL || "http://localhost:3000";

export const WS_URL = API_URL.replace(/^http/, "ws") + "/ws";

export async function request(path, options = {}) {
  const token = options.token || getStoredToken();
  const headers = {
    "Content-Type": "application/json",
    ...(token ? { Authorization: `Bearer ${token}` } : {}),
    ...(options.headers || {}),
  };

  const response = await fetch(`${API_URL}${path}`, {
    ...options,
    headers,
  });

  if (response.status === 401 && options.onUnauthorized) {
    options.onUnauthorized();
  }

  let data = null;
  const contentType = response.headers.get("content-type") || "";
  if (contentType.includes("application/json")) {
    try {
      data = await response.json();
    } catch {
      data = null;
    }
  }

  if (!response.ok) {
    const error = new Error(
      data?.error || `Request failed with status ${response.status}`,
    );
    error.status = response.status;
    error.data = data;
    throw error;
  }

  return data;
}

export function getStoredToken() {
  try {
    const raw = sessionStorage.getItem("sankat-net-session");
    if (!raw) return null;
    const parsed = JSON.parse(raw);
    return parsed?.token || null;
  } catch {
    return null;
  }
}
