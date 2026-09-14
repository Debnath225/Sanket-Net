import React from "react";
import { Database, ShieldCheck } from "lucide-react";

export function EmptyState({
  icon: Icon = Database,
  title = "No data available",
  description = "Waiting for live gateway records",
  action,
}) {
  return (
    <div className="empty-state">
      <div className="empty-icon-wrap">
        <Icon size={26} />
      </div>
      <h4 className="empty-title">{title}</h4>
      <p className="empty-desc">{description}</p>
      {action && <div className="empty-action">{action}</div>}
    </div>
  );
}
