# Sankat-Net Backend

Node.js service for the `Gateway -> MQTT -> backend -> dashboard` path.

## Responsibilities

- Subscribe to `sankatnet/+/+` MQTT topics.
- Persist telemetry, alerts, heartbeats, and events in PostgreSQL when `DATABASE_URL` is configured.
- Fall back to `data/telemetry.json` only when no database URL is configured.
- Run an explainable AI hazard predictor for weather, flood, landslide, forest fire, seismic, and pollution signals.
- Expose REST endpoints for the dashboard.
- Broadcast live records over WebSocket.

## Run

```powershell
npm install
Copy-Item .env.example .env
npm start
```

Set `DATABASE_URL` in `.env` before starting. Prisma manages the schema in `prisma/schema.prisma`:

```env
DATABASE_URL=postgresql://postgres:password@localhost:5432/sankatnet
AUTH_USERNAME=operator
AUTH_PASSWORD=use-a-long-unique-password
AUTH_TOKEN_SECRET=generate-at-least-32-random-characters
```

The dashboard requires these operator credentials before it can access the
telemetry API or live WebSocket feed. Generate `AUTH_TOKEN_SECRET` with:

```bash
node -e "console.log(require('crypto').randomBytes(48).toString('base64url'))"
```

Initialize or update the database with:

```powershell
npx prisma generate
npx prisma db push
```

For hosted PostgreSQL, use the provider connection string, including its required SSL parameters.

The service listens on `http://localhost:4000` by default.

Endpoints:

- `GET /api/health` is public for service monitoring.
- `POST /api/auth/login` accepts the configured username and password.
- All remaining `/api/*` endpoints require `Authorization: Bearer <token>`.
- The WebSocket requires `ws://localhost:4000/ws?token=<token>`.

Authenticated dashboard API:

- `GET /api/snapshot`
- `GET /api/telemetry?limit=100`, `GET /api/alerts?limit=100`, and `GET /api/events?limit=100`
- `GET /api/nodes`, `GET /api/nodes/:nodeId`, and `GET /api/network/summary`
- `POST /api/predict` and `POST /api/ingest` for controlled integration use

Set `MQTT_URL`, credentials, and `MQTT_CA_FILE` in `.env`. TLS broker connections should use a CA file in production.

Prisma Studio is available with:

```powershell
npm run db:studio
```

## RAG assistant

`POST /api/chat` is connected to the dashboard's **Ask Sankat** control. It retrieves and ranks recent live telemetry, alerts, heartbeats, and events locally, and returns the records used under `sources`.

To enable an LLM-written response after retrieval, configure an OpenAI-compatible chat-completions provider in `Backend/.env`:

```env
LLM_API_KEY=your-provider-key
LLM_BASE_URL=https://api.openai.com/v1
LLM_MODEL=gpt-4.1-mini
```

Without `LLM_API_KEY`, the backend still provides a grounded local answer. Keep the API key only in `Backend/.env`, never in the frontend.

## Administration

The backend uses ADMIN, OPERATOR, and VIEWER roles. The configured AUTH_USERNAME signs in as an administrator. Set ALLOW_REGISTRATION=true only when you intentionally need to create new operator accounts. Chat conversations are persisted in PostgreSQL when DATABASE_URL is configured.
