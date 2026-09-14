import "dotenv/config";

const required = (name) => process.env[name] || "";

export const config = {
  port: Number(process.env.PORT || 4000),
  mqttUrl: required("MQTT_URL"),
  mqttUsername: required("MQTT_USERNAME"),
  mqttPassword: required("MQTT_PASSWORD"),
  mqttCaFile: required("MQTT_CA_FILE"),
  mqttClientId:
    process.env.MQTT_CLIENT_ID || `sankatnet-backend-${process.pid}`,
  mqttTopicRoot: process.env.MQTT_TOPIC_ROOT || "sankatnet",
  corsOrigins: (
    process.env.CORS_ORIGIN || "http://localhost:5173,http://localhost:5174"
  )
    .split(",")
    .map((origin) => origin.trim())
    .filter(Boolean),
  dataFile: process.env.DATA_FILE || "./data/telemetry.json",
  databaseUrl: required("DATABASE_URL"),
  authUsername: required("AUTH_USERNAME"),
  authPassword: required("AUTH_PASSWORD"),
  authTokenSecret: required("AUTH_TOKEN_SECRET"),
  authTokenTtlSeconds: Number(process.env.AUTH_TOKEN_TTL_SECONDS || 28800),
  allowRegistration: process.env.ALLOW_REGISTRATION === "true",
  adminRegistrationCode: required("ADMIN_REGISTRATION_CODE"),
  llmApiKey: required("LLM_API_KEY"),
  llmBaseUrl: process.env.LLM_BASE_URL || "https://api.openai.com/v1",
  llmModel: process.env.LLM_MODEL || "gpt-4.1-mini",
  llmTimeoutMs: Number(process.env.LLM_TIMEOUT_MS || 15000),
};

if (
  !config.authUsername ||
  !config.authPassword ||
  !config.authTokenSecret ||
  config.authTokenSecret.length < 32 ||
  !Number.isFinite(config.authTokenTtlSeconds) ||
  config.authTokenTtlSeconds < 60
) {
  throw new Error(
    "Authentication requires AUTH_USERNAME, AUTH_PASSWORD, and an AUTH_TOKEN_SECRET of at least 32 characters.",
  );
}
