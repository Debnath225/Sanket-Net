import React from "react";

export function StatCard({ icon: Icon, label, value, detail, tone = "cyan", trend }) {
  return (
    <div className={`stat-card tone-${tone}`}>
      <div className="stat-card-glow" />
      <div className="stat-header">
        <span className="stat-label">{label}</span>
        {Icon && (
          <div className="stat-icon-wrap">
            <Icon size={18} />
          </div>
        )}
      </div>
      <div className="stat-body">
        <div className="stat-value">{value}</div>
        {(detail || trend) && (
          <div className="stat-footer">
            {trend && <span className="stat-trend">{trend}</span>}
            {detail && <span className="stat-detail">{detail}</span>}
          </div>
        )}
      </div>
    </div>
  );
}
