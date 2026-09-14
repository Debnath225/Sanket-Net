import React, { useEffect, useState } from "react";
import { Check, Edit, Shield, ToggleLeft, ToggleRight, UserCheck, UserX, Users, X } from "lucide-react";
import { useAuth } from "../../context/AuthContext.jsx";
import { endpoints } from "../../api/endpoints.js";
import { Panel } from "../../components/common/Panel.jsx";
import { EmptyState } from "../../components/common/EmptyState.jsx";
import { Modal } from "../../components/common/Modal.jsx";

export function AdminUsers() {
  const { session, user: currentUser } = useAuth();
  const token = session?.token;
  const [users, setUsers] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");
  const [editingUser, setEditingUser] = useState(null);
  const [selectedRole, setSelectedRole] = useState("OPERATOR");
  const [selectedActive, setSelectedActive] = useState(true);
  const [saving, setSaving] = useState(false);

  const fetchUsers = async () => {
    if (!token) return;
    setLoading(true);
    setError("");
    try {
      const data = await endpoints.getAdminUsers(token);
      setUsers(Array.isArray(data) ? data : []);
    } catch (err) {
      setError("Unable to load user accounts from backend.");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchUsers();
  }, [token]);

  const handleEditClick = (user) => {
    setEditingUser(user);
    setSelectedRole(user.role || "OPERATOR");
    setSelectedActive(Boolean(user.active));
  };

  const handleSaveUser = async (e) => {
    e.preventDefault();
    if (!editingUser || !token) return;
    setSaving(true);
    try {
      await endpoints.updateAdminUser(
        editingUser.username,
        { role: selectedRole, active: selectedActive },
        token,
      );
      setEditingUser(null);
      await fetchUsers();
    } catch (err) {
      alert("Failed to update user. Note: you cannot deactivate your own account.");
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="admin-page-view">
      <Panel
        icon={Users}
        title="User Access &amp; Permissions"
        subtitle="Manage operators, citizens, and administrative privilege levels"
        action={
          <button className="admin-primary-btn" onClick={fetchUsers} disabled={loading}>
            Refresh Users
          </button>
        }
      >
        {error ? (
          <div className="admin-error-box">{error}</div>
        ) : users.length > 0 ? (
          <div className="admin-table-wrapper">
            <table className="admin-table">
              <thead>
                <tr>
                  <th>Username</th>
                  <th>Assigned Role</th>
                  <th>Status</th>
                  <th>Account Created</th>
                  <th>Actions</th>
                </tr>
              </thead>
              <tbody>
                {users.map((u) => {
                  const isSelf = u.username === currentUser?.username;
                  return (
                    <tr key={u.username}>
                      <td>
                        <div className="user-id-cell">
                          <strong>{u.username}</strong>
                          {isSelf && <span className="self-tag">(Current Session)</span>}
                        </div>
                      </td>
                      <td>
                        <span className={`role-pill role-${u.role?.toLowerCase()}`}>
                          {u.role || "OPERATOR"}
                        </span>
                      </td>
                      <td>
                        <span className={`status-tag ${u.active ? "active" : "inactive"}`}>
                          {u.active ? "Active" : "Disabled"}
                        </span>
                      </td>
                      <td>
                        <span>
                          {u.createdAt ? new Date(u.createdAt).toLocaleDateString() : "System"}
                        </span>
                      </td>
                      <td>
                        <button
                          className="admin-action-btn"
                          onClick={() => handleEditClick(u)}
                        >
                          Modify Access
                        </button>
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        ) : (
          <EmptyState
            icon={Users}
            title={loading ? "Loading User Accounts..." : "No User Accounts Found"}
            description={
              loading
                ? "Contacting backend user registry..."
                : "Registered operator and citizen accounts will appear here."
            }
          />
        )}
      </Panel>

      {/* Edit User Modal */}
      <Modal
        isOpen={Boolean(editingUser)}
        onClose={() => setEditingUser(null)}
        title={`Modify User: ${editingUser?.username}`}
        subtitle="Update access role or toggle account active state"
      >
        {editingUser && (
          <form onSubmit={handleSaveUser} className="admin-form">
            <div className="form-group">
              <label>Role Privilege</label>
              <select
                value={selectedRole}
                onChange={(e) => setSelectedRole(e.target.value)}
              >
                <option value="ADMIN">ADMIN (Full Command Center Access)</option>
                <option value="OPERATOR">OPERATOR (Standard Field User)</option>
                <option value="VIEWER">VIEWER (Read-Only Public Citizen)</option>
              </select>
            </div>

            <div className="form-group checkbox-group">
              <label className="toggle-label">
                <input
                  type="checkbox"
                  checked={selectedActive}
                  disabled={editingUser.username === currentUser?.username}
                  onChange={(e) => setSelectedActive(e.target.checked)}
                />
                <span>Account Active &amp; Allowed to Sign In</span>
              </label>
              {editingUser.username === currentUser?.username && (
                <small className="form-help-text">
                  You cannot deactivate your own active session.
                </small>
              )}
            </div>

            <button
              type="submit"
              className="admin-primary-btn full"
              disabled={saving}
            >
              {saving ? "Updating..." : "Save User Changes"}
            </button>
          </form>
        )}
      </Modal>
    </div>
  );
}
