import fs from "node:fs/promises";
import path from "node:path";
import { PrismaClient } from "@prisma/client";
const COLLECTIONS = ["telemetry", "alerts", "heartbeats", "events"];

export class Store {
  constructor(fileName, databaseUrl) {
    this.fileName = path.resolve(fileName);
    this.prisma = databaseUrl ? new PrismaClient() : null;
    this.state = Object.fromEntries(COLLECTIONS.map((name) => [name, []]));
    this.clients = new Set();
  }

  get persistenceMode() {
    return this.prisma ? "prisma-postgres" : "json-fallback";
  }

  async init() {
    if (this.prisma) {
      await this.prisma.$connect();
      const rows = await this.prisma.record.findMany({
        orderBy: { createdAt: "desc" },
        take: 2000,
        select: { kind: true, payload: true },
      });
      for (const row of rows) {
        if (this.state[row.kind].length < 500)
          this.state[row.kind].push(row.payload);
      }
      return;
    }

    await fs.mkdir(path.dirname(this.fileName), { recursive: true });
    try {
      this.state = {
        ...this.state,
        ...JSON.parse(await fs.readFile(this.fileName, "utf8")),
      };
    } catch (error) {
      if (error.code !== "ENOENT") throw error;
      await this.persistJson();
    }
  }

  async persistJson() {
    const temporary = `${this.fileName}.tmp`;
    await fs.writeFile(temporary, JSON.stringify(this.state, null, 2));
    await fs.rename(temporary, this.fileName);
  }

  async add(kind, value) {
    if (!COLLECTIONS.includes(kind))
      throw new Error(`Unknown store collection: ${kind}`);
    this.state[kind].unshift(value);
    this.state[kind] = this.state[kind].slice(0, 500);
    this.broadcast({ type: kind, data: value });
    if (this.prisma) {
      await this.prisma.record.create({
        data: { kind, payload: value },
      });
    } else {
      await this.persistJson();
    }
  }

  async close() {
    await this.prisma?.$disconnect();
  }

  addClient(client) {
    this.clients.add(client);
    client.on("close", () => this.clients.delete(client));
  }

  broadcast(message) {
    const encoded = JSON.stringify(message);
    for (const client of this.clients) {
      if (client.readyState === 1) client.send(encoded);
    }
  }

  snapshot() {
    return {
      telemetry: this.state.telemetry.slice(0, 100),
      alerts: this.state.alerts.slice(0, 100),
      heartbeats: this.state.heartbeats.slice(0, 100),
      events: this.state.events.slice(0, 100),
    };
  }
}
