import express from "express";
import crypto from "node:crypto";
import http from "node:http";
import { WebSocketServer } from "ws";
import { config } from "./config.js";
import { Store } from "./store.js";
import { startMqtt } from "./mqtt.js";
import { createAuth } from "./auth.js";
import { cors } from "./middleware/cors.js";
import { createDashboardRouter } from "./routes/dashboardRoutes.js";

const store = new Store(config.dataFile, config.databaseUrl);
await store.init();
const auth = createAuth(config);
const app = express();
app.use(express.json({ limit: "64kb" }));
app.use(cors(config.corsOrigins));

app.get("/api/health", (_request, response) =>
  response.json({
    ok: true,
    service: "sankat-net-backend",
    persistence: store.persistenceMode,
    at: new Date().toISOString(),
  }),
);
const validUsername = (username) => /^[a-zA-Z0-9_.-]{3,64}$/.test(username);
const validPassword = (password) => typeof password === "string" && password.length >= 8 && password.length <= 256;
const sessionResponse = (response, username, role = "ADMIN") => {
  const session = auth.issueToken(username, role);
  return response.json({ ...session, user: { username, role } });
};

const validAccountType = (value) => value === "ADMIN" || value === "OPERATOR";
const enrollmentCodeMatches = (candidate, expected) => {
  if (!expected || typeof candidate !== "string") return false;
  const left = Buffer.from(candidate);
  const right = Buffer.from(expected);
  return left.length === right.length && crypto.timingSafeEqual(left, right);
};

app.post("/api/auth/register", async (request, response, next) => {
  if (!config.allowRegistration) return response.status(403).json({ error: "registration_disabled" });
  const { username, password, accountType = "OPERATOR", adminCode } = request.body || {};
  if (typeof username !== "string" || !validUsername(username) || !validPassword(password) || !validAccountType(accountType))
    return response.status(400).json({ error: "invalid_registration" });
  if (accountType === "ADMIN" && !enrollmentCodeMatches(adminCode, config.adminRegistrationCode))
    return response.status(403).json({ error: "invalid_admin_enrollment_code" });
  try {
    const user = await store.createUser(username, await auth.hashPassword(password), accountType);
    return sessionResponse(response, user.username, user.role);
  } catch (error) {
    if (error.code === "P2002") return response.status(409).json({ error: "username_taken" });
    if (error.code === "P2021" || error.code === "USER_DATABASE_UNAVAILABLE") return response.status(503).json({ error: "registration_unavailable" });
    return next(error);
  }
});
app.post("/api/auth/login", async (request, response, next) => {
  const { username, password, accountType } = request.body || {};
  if (typeof username !== "string" || typeof password !== "string")
    return response.status(401).json({ error: "invalid_credentials" });
  try {
    const user = await store.findUserByUsername(username);
    if (user?.active && (await auth.verifyPassword(password, user.passwordHash)))
      return sessionResponse(response, user.username, user.role);
    if (auth.authenticate(username, password)) return sessionResponse(response, config.authUsername, "ADMIN");
    return response.status(401).json({ error: "invalid_credentials" });
  } catch (error) {
    if (auth.authenticate(username, password)) return sessionResponse(response, config.authUsername, "ADMIN");
    if (error.code === "P2021") return response.status(503).json({ error: "registration_unavailable" });
    return next(error);
  }
});
app.use("/api", auth.requireAuth);
app.use("/api", async (request, response, next) => {
  if (request.session.sub === config.authUsername) return next();
  try {
    const user = await store.findUserByUsername(request.session.sub);
    if (!user?.active) return response.status(401).json({ error: "account_inactive" });
    request.session.role = user.role;
    return next();
  } catch (error) { return next(error); }
});
app.use("/api", createDashboardRouter(store, config));

app.use((error, _request, response, _next) => {
  console.error(error);
  response.status(500).json({ error: "internal_server_error" });
});

const server = http.createServer(app);
let startupFailureHandled = false;
const handleListenError = async (error) => {
  if (startupFailureHandled) return;
  startupFailureHandled = true;

  if (error.code === "EADDRINUSE") {
    console.error(
      `[startup] Port ${config.port} is already in use. ` +
        "Stop the existing backend or set PORT to another value.",
    );
  } else {
    console.error("[startup] Server failed:", error);
  }
  await store.close();
  process.exitCode = 1;
};

server.on("error", handleListenError);
const websocket = new WebSocketServer({ noServer: true });
websocket.on("error", handleListenError);
websocket.on("connection", (socket) => {
  store.addClient(socket);
  socket.send(JSON.stringify({ type: "snapshot", data: store.snapshot() }));
});
server.on("upgrade", (request, socket, head) => {
  const requestUrl = new URL(request.url || "/", `http://${request.headers.host}`);
  if (requestUrl.pathname !== "/ws") return socket.destroy();
  const session = auth.verifyToken(requestUrl.searchParams.get("token"));
  if (!session) {
    socket.write("HTTP/1.1 401 Unauthorized\r\nConnection: close\r\n\r\n");
    return socket.destroy();
  }
  websocket.handleUpgrade(request, socket, head, (client) => {
    websocket.emit("connection", client, request, session);
  });
});
startMqtt(config, store);
server.listen(config.port, () =>
  console.log(
    `Sankat-Net backend listening on http://localhost:${config.port}`,
  ),
);

process.on("SIGTERM", async () => {
  await store.close();
  server.close(() => process.exit(0));
});
