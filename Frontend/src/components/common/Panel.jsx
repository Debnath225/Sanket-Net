import React from "react";

export function Panel({ icon: Icon, title, subtitle, action, children, className = "" }) {
  return (
    <section className={`panel-card ${className}`}>
      {(title || Icon || action) && (
        <header className="panel-header">
          <div className="panel-title-group">
            {Icon && (
              <div className="panel-icon-wrap">
                <Icon size={17} />
              </div>
            )}
            <div>
              {title && <h3 className="panel-title">{title}</h3>}
              {subtitle && <p className="panel-subtitle">{subtitle}</p>}
            </div>
          </div>
          {action && <div className="panel-action">{action}</div>}
        </header>
      )}
      <div className="panel-content">{children}</div>
    </section>
  );
}
