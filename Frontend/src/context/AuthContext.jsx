import React, { createContext, useContext, useEffect, useState } from "react";
import { loadSession, login as apiLogin, register as apiRegister, saveSession } from "../api/authApi.js";

const AuthContext = createContext(null);

export function AuthProvider({ children }) {
  const [session, setSession] = useState(loadSession);
  const [loading, setLoading] = useState(false);

  const user = session?.user || null;
  const role = user?.role || null;
  const isAdmin = role === "ADMIN";
  const isAuthenticated = Boolean(session?.token);

  const login = async ({ username, password, accountType }) => {
    setLoading(true);
    try {
      const data = await apiLogin({ username, password, accountType });
      saveSession(data);
      setSession(data);
      return data;
    } finally {
      setLoading(false);
    }
  };

  const register = async ({ username, password, accountType, adminCode }) => {
    setLoading(true);
    try {
      const data = await apiRegister({ username, password, accountType, adminCode });
      saveSession(data);
      setSession(data);
      return data;
    } finally {
      setLoading(false);
    }
  };

  const logout = () => {
    saveSession(null);
    setSession(null);
  };

  return (
    <AuthContext.Provider
      value={{
        session,
        user,
        role,
        isAdmin,
        isAuthenticated,
        loading,
        login,
        register,
        logout,
      }}
    >
      {children}
    </AuthContext.Provider>
  );
}

export function useAuth() {
  const context = useContext(AuthContext);
  if (!context) {
    throw new Error("useAuth must be used within an AuthProvider");
  }
  return context;
}
