import express from "express";
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
app.post("/api/auth/login", (request, response) => {
  const { username, password } = request.body || {};
  if (!auth.authenticate(username, password))
    return response.status(401).json({ error: "invalid_credentials" });
  const session = auth.issueToken();
  response.json({
    ...session,
    user: { username: config.authUsername, role: "operator" },
  });
});
app.use("/api", auth.requireAuth);
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
