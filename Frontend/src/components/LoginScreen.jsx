import React, { useState } from "react";
import { Eye, EyeOff, LockKeyhole, Radio } from "lucide-react";
import { API_URL } from "../data.js";

export function LoginScreen({ onAuthenticated }) {
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [showPassword, setShowPassword] = useState(false);
  const [error, setError] = useState("");
  const [submitting, setSubmitting] = useState(false);

  const submit = async (event) => {
    event.preventDefault();
    setError("");
    setSubmitting(true);
    try {
      const response = await fetch(`${API_URL}/api/auth/login`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ username, password }),
      });
      const data = await response.json().catch(() => ({}));
      if (!response.ok || !data.token) {
        setError(
          response.status === 401
            ? "Incorrect username or password."
            : "Unable to reach the authentication service.",
        );
        return;
      }
      onAuthenticated(data);
    } catch {
      setError("Unable to reach the authentication service.");
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <main className="login-page">
      <section className="login-card" aria-labelledby="login-title">
        <div className="login-brand">
          <div className="brand-mark">SN</div>
          <div>
            <strong>Sankat-Net</strong>
            <span>Environmental intelligence</span>
          </div>
        </div>
        <div className="login-copy">
          <span className="login-icon"><LockKeyhole size={19} /></span>
          <p className="eyebrow accent">SECURE OPERATOR ACCESS</p>
          <h1 id="login-title">Sign in to the dashboard</h1>
          <p>Use the operator credentials configured on the backend.</p>
        </div>
        <form className="login-form" onSubmit={submit}>
          <label>
            Username
            <input
              autoComplete="username"
              value={username}
              onChange={(event) => setUsername(event.target.value)}
              required
              maxLength="128"
            />
          </label>
          <label>
            Password
            <span className="password-input">
              <input
                type={showPassword ? "text" : "password"}
                autoComplete="current-password"
                value={password}
                onChange={(event) => setPassword(event.target.value)}
                required
                maxLength="256"
              />
              <button
                type="button"
                onClick={() => setShowPassword((current) => !current)}
                aria-label={showPassword ? "Hide password" : "Show password"}
              >
                {showPassword ? <EyeOff size={16} /> : <Eye size={16} />}
              </button>
            </span>
          </label>
          {error && <p className="login-error" role="alert">{error}</p>}
          <button className="login-submit" disabled={submitting}>
            <Radio size={16} /> {submitting ? "Signing in…" : "Sign in securely"}
          </button>
        </form>
      </section>
    </main>
  );
}
