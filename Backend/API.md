# Sankat-Net Backend API

Base URL: `http://localhost:4000`. Request bodies are JSON. Every endpoint except health and authentication requires `Authorization: Bearer <token>`.

## Authentication

| Method | Path | Purpose |
| --- | --- | --- |
| `GET` | `/api/health` | Public service and persistence health. |
| `POST` | `/api/auth/register` | Create a database-backed operator account. Body: `username`, `password` (8-256 chars). |
| `POST` | `/api/auth/login` | Sign in with an operator account or the `AUTH_USERNAME`/`AUTH_PASSWORD` fallback. Body: `username`, `password`. |

Successful login/register response: `{ "token": "â€¦", "expiresAt": "ISO-8601", "user": { "username": "â€¦", "role": "operator" } }`.

## Dashboard and telemetry

| Method | Path | Purpose |
| --- | --- | --- |
| `GET` | `/api/snapshot` | Up to 100 most-recent telemetry, alerts, heartbeats, and events. |
| `GET` | `/api/telemetry?limit=100` | Recent gateway telemetry held in the live store. Limit: 1-500. |
| `GET` | `/api/sensor-readings?limit=100&nodeId=NODE-01` | Typed historical sensor rows from PostgreSQL. `nodeId` is optional. |
| `GET` | `/api/alerts?limit=100` | Recent alerts. |
| `GET` | `/api/events?limit=100` | Recent system events. |
| `GET` | `/api/nodes` | Latest status derived per reporting node. |
| `GET` | `/api/nodes/:nodeId` | One node status; returns 404 when absent. |
| `GET` | `/api/network/summary` | Aggregate node, alert, and event counts. |

A `SensorReading` stores node/time/location/battery and typed numeric metrics (`temperature`, `humidity`, `rainfallMm`, `waterLevel`, `soilMoisture`, `windSpeed`, `vibration`, `mq2`, `mq135`, `turbidity`), risk prediction, raw gateway payload, search document, and a normalized 256-number embedding.

## Ingestion and prediction

| Method | Path | Purpose |
| --- | --- | --- |
| `POST` | `/api/ingest` | Add manual telemetry. The body is stored as a telemetry record, converted to `SensorReading`, embedded, predicted, and broadcast. Optional header: `x-mqtt-topic`. |
| `POST` | `/api/predict` | Run the explainable hazard predictor without persisting. Body may include the same sensor measurements. |

Example ingestion:

```json
{
  "node_id": "NODE-01",
  "temperature": 28.4,
  "humidity": 91,
  "rainfall_mm": 48,
  "water_level": 810,
  "soil_moisture": 88,
  "battery_mv": 3980
}
```

## RAG assistant

| Method | Path | Purpose |
| --- | --- | --- |
| `POST` | `/api/chat` | Retrieves the best matching persisted sensor vectors plus live alerts/events, then returns a grounded answer. Body: `{ "question": "What is the flood risk?" }`. |

The response contains `answer`, `sources` (kind, summary, timestamp, score), `provider` (`local` or `llm`), and `retrievedAt`. `LLM_API_KEY` enables the optional OpenAI-compatible response writer. Without it, the answer is generated locally and sensor data is not sent to any external provider.

## Live WebSocket

Connect to `ws://localhost:4000/ws?token=<token>`. The server sends an initial `snapshot`, then messages such as `{ "type": "telemetry", "data": { â€¦ } }`, `alerts`, `heartbeats`, and `events`.

## Database setup and demo data

```powershell
cd Backend
npm install
npm run db:generate
npm run db:push
npm run db:seed
```

`db:seed` removes and recreates only rows whose node ID begins with `DEMO-`; it adds flood, landslide, and air/fire-risk example readings. Do not run it against a production database unless those demo rows are acceptable.

## Conversations and admin control

Chat requests now support an optional `conversationId`. When PostgreSQL is configured, each user question and assistant answer is stored transactionally in `chat_conversations` and `chat_messages`, including response sources and provider metadata.

| Method | Path | Access | Purpose |
| --- | --- | --- | --- |
| `GET` | `/api/conversations` | Signed-in user | List the caller's saved conversations. |
| `GET` | `/api/conversations/:id` | Owner | Retrieve a conversation and its chronological messages. |
| `DELETE` | `/api/conversations/:id` | Owner | Delete one conversation and its messages. |
| `GET` | `/api/admin/status` | Admin | Safe service configuration status; secrets are never returned. |
| `GET` | `/api/admin/users` | Admin | List user roles and active states. |
| `PATCH` | `/api/admin/users/:username` | Admin | Change `{ "role": "ADMIN|OPERATOR|VIEWER", "active": true|false }`. |
| `GET` | `/api/admin/conversations` | Admin | List all saved conversations. |
| `DELETE` | `/api/admin/conversations/:id` | Admin | Remove any saved conversation. |

The configured `AUTH_USERNAME` account has `ADMIN` access. Database-backed accounts are `OPERATOR` by default. Set `ALLOW_REGISTRATION=true` temporarily only while onboarding users. `POST /api/auth/register` accepts `accountType` (`OPERATOR` or `ADMIN`); `ADMIN` additionally requires the server-only `ADMIN_REGISTRATION_CODE`.
