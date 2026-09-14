import React, { useState } from "react";
import {
  AlertCircle,
  CheckCircle,
  Eye,
  EyeOff,
  KeyRound,
  Lock,
  Radio,
  ShieldAlert,
  ShieldCheck,
  User,
  Users,
} from "lucide-react";
import { useAuth } from "../../context/AuthContext.jsx";

export function AuthScreen() {
  const { login, register } = useAuth();
  const [mode, setMode] = useState("login"); // "login" | "signup"
  const [accountType, setAccountType] = useState("OPERATOR"); // "OPERATOR" (User) | "ADMIN"
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [adminCode, setAdminCode] = useState("");
  const [showPassword, setShowPassword] = useState(false);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError("");
    setLoading(true);

    try {
      if (mode === "login") {
        await login({ username, password, accountType });
      } else {
        await register({ username, password, accountType, adminCode });
      }
    } catch (err) {
      let msg = "An unexpected authentication error occurred.";
      if (err.status === 401) {
        msg = "Invalid username or password. Please verify your credentials.";
      } else if (err.status === 403) {
        if (err.data?.error === "invalid_admin_enrollment_code") {
          msg = "Invalid Administrator Enrollment Code. Contact the system authority.";
        } else if (err.data?.error === "registration_disabled") {
          msg = "Registration is currently disabled by system policy.";
        } else {
          msg = "Access denied for this role.";
        }
      } else if (err.status === 409 || err.data?.error === "username_taken") {
        msg = "That username is already taken. Please pick another.";
      } else if (err.status === 400) {
        msg = "Username must be 3-64 characters and password at least 8 characters.";
      } else if (err.status === 503) {
        msg = "Registration service is temporarily unavailable.";
      }
      setError(msg);
    } finally {
      setLoading(false);
    }
  };

  const autofillAdmin = () => {
    setMode("login");
    setAccountType("ADMIN");
    setUsername("operator");
    setPassword("SankatNet-Operator-2026!");
    setError("");
  };

  return (
    <div className="auth-page-wrapper">
      <div className="auth-background-glow glow-1" />
      <div className="auth-background-glow glow-2" />

      <div className="auth-card-container">
        <div className="auth-card">
          <header className="auth-card-header">
            <div className="auth-brand-badge">
              <ShieldCheck size={28} />
            </div>
            <h1 className="auth-title">Sankat-Net</h1>
            <p className="auth-subtitle">
              Disaster Early Warning & Community Resilience Network
            </p>
          </header>

          {/* Mode Tabs: Login vs Register */}
          <div className="auth-tab-row">
            <button
              type="button"
              className={`auth-tab-btn ${mode === "login" ? "active" : ""}`}
              onClick={() => {
                setMode("login");
                setError("");
              }}
            >
              Sign In
            </button>
            <button
              type="button"
              className={`auth-tab-btn ${mode === "signup" ? "active" : ""}`}
              onClick={() => {
                setMode("signup");
                setError("");
              }}
            >
              Create Account
            </button>
          </div>

          {/* Role Selector: User vs Admin */}
          <div className="auth-role-selector">
            <button
              type="button"
              className={`role-option-card ${accountType === "OPERATOR" ? "selected" : ""}`}
              onClick={() => {
                setAccountType("OPERATOR");
                setError("");
              }}
            >
              <div className="role-icon-wrap user">
                <Users size={20} />
              </div>
              <div className="role-meta">
                <strong>Citizen / User</strong>
                <small>Public safety alerts & survival guides</small>
              </div>
            </button>

            <button
              type="button"
              className={`role-option-card ${accountType === "ADMIN" ? "selected" : ""}`}
              onClick={() => {
                setAccountType("ADMIN");
                setError("");
              }}
            >
              <div className="role-icon-wrap admin">
                <Radio size={20} />
              </div>
              <div className="role-meta">
                <strong>Administrator</strong>
                <small>Command center & network operations</small>
              </div>
            </button>
          </div>

          {error && (
            <div className="auth-error-banner" role="alert">
              <AlertCircle size={16} />
              <span>{error}</span>
            </div>
          )}

          <form className="auth-form" onSubmit={handleSubmit}>
            <div className="form-group">
              <label htmlFor="auth-username">
                <User size={14} />
                <span>Username</span>
              </label>
              <input
                id="auth-username"
                type="text"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                placeholder={
                  accountType === "ADMIN" ? "admin_username" : "citizen_user"
                }
                required
                minLength={3}
                maxLength={64}
                autoComplete="username"
                disabled={loading}
              />
            </div>

            <div className="form-group">
              <label htmlFor="auth-password">
                <Lock size={14} />
                <span>Password</span>
              </label>
              <div className="password-input-wrapper">
                <input
                  id="auth-password"
                  type={showPassword ? "text" : "password"}
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  placeholder="At least 8 characters"
                  required
                  minLength={8}
                  maxLength={256}
                  autoComplete={
                    mode === "signup" ? "new-password" : "current-password"
                  }
                  disabled={loading}
                />
                <button
                  type="button"
                  className="password-toggle"
                  onClick={() => setShowPassword((p) => !p)}
                  aria-label={showPassword ? "Hide password" : "Show password"}
                >
                  {showPassword ? <EyeOff size={16} /> : <Eye size={16} />}
                </button>
              </div>
            </div>

            {mode === "signup" && accountType === "ADMIN" && (
              <div className="form-group">
                <label htmlFor="auth-admin-code">
                  <KeyRound size={14} />
                  <span>Administrator Enrollment Code</span>
                </label>
                <input
                  id="auth-admin-code"
                  type="password"
                  value={adminCode}
                  onChange={(e) => setAdminCode(e.target.value)}
                  placeholder="Enter authorized server code (e.g. SANKAT-ADMIN-2026)"
                  required
                  autoComplete="one-time-code"
                  disabled={loading}
                />
                <small className="form-help-text">
                  Enrollment codes are issued exclusively to authorized field directors.
                </small>
              </div>
            )}

            <button
              type="submit"
              className={`auth-submit-btn ${accountType === "ADMIN" ? "admin-btn" : "user-btn"}`}
              disabled={loading}
            >
              {loading
                ? "Verifying Credentials..."
                : mode === "login"
                  ? `Enter ${accountType === "ADMIN" ? "Admin Command Center" : "Citizen Safety Hub"}`
                  : `Register ${accountType === "ADMIN" ? "Administrator" : "Citizen"} Account`}
            </button>
          </form>

          <footer className="auth-card-footer">
            <button
              type="button"
              className="demo-credentials-btn"
              onClick={autofillAdmin}
            >
              Auto-fill System Admin Demo Credentials
            </button>
          </footer>
        </div>
      </div>
    </div>
  );
}
