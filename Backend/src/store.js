import fs from "node:fs/promises";
import path from "node:path";
import crypto from "node:crypto";
import { PrismaClient } from "@prisma/client";
import { cosineSimilarity, createEmbedding } from "./ai/rag.js";
import { toSensorReading } from "./ai/sensorReading.js";
const COLLECTIONS = ["telemetry", "alerts", "heartbeats", "events"];

export class Store {
  constructor(fileName, databaseUrl) { this.fileName = path.resolve(fileName); this.prisma = databaseUrl ? new PrismaClient() : null; this.state = Object.fromEntries(COLLECTIONS.map((name) => [name, []])); this.clients = new Set(); }
  get persistenceMode() { return this.prisma ? "prisma-postgres" : "json-fallback"; }
  async init() {
    if (this.prisma) { await this.prisma.$connect(); const rows = await this.prisma.record.findMany({ orderBy: { createdAt: "desc" }, take: 2000, select: { kind: true, payload: true } }); for (const row of rows) if (this.state[row.kind].length < 500) this.state[row.kind].push(row.payload); return; }
    await fs.mkdir(path.dirname(this.fileName), { recursive: true }); try { this.state = { ...this.state, ...JSON.parse(await fs.readFile(this.fileName, "utf8")) }; } catch (error) { if (error.code !== "ENOENT") throw error; await this.persistJson(); }
  }
  async persistJson() { const temporary = `${this.fileName}.tmp`; await fs.writeFile(temporary, JSON.stringify(this.state, null, 2)); await fs.rename(temporary, this.fileName); }
  async add(kind, value) {
    if (!COLLECTIONS.includes(kind)) throw new Error(`Unknown store collection: ${kind}`);
    this.state[kind].unshift(value); this.state[kind] = this.state[kind].slice(0, 500); this.broadcast({ type: kind, data: value });
    if (this.prisma) {
      const record = { data: { kind, payload: value } };
      if (kind === "telemetry") await this.prisma.$transaction([this.prisma.record.create(record), this.prisma.sensorReading.create({ data: toSensorReading(value) })]);
      else await this.prisma.record.create(record);
    } else await this.persistJson();
  }
  async searchSensorReadings(question, limit = 8) {
    if (!this.prisma) return [];
    const queryEmbedding = createEmbedding(question);
    const rows = await this.prisma.sensorReading.findMany({ orderBy: { capturedAt: "desc" }, take: 2000, select: { rawPayload: true, embedding: true } });
    return rows.map((row) => ({ kind: "telemetry", record: row.rawPayload, score: cosineSimilarity(queryEmbedding, Array.isArray(row.embedding) ? row.embedding : []) })).filter((row) => row.score > 0).sort((a, b) => b.score - a.score).slice(0, limit);
  }
  async listSensorReadings({ limit = 100, nodeId } = {}) {
    if (!this.prisma) return this.state.telemetry.slice(0, limit);
    const rows = await this.prisma.sensorReading.findMany({ where: nodeId ? { nodeId } : undefined, orderBy: { capturedAt: "desc" }, take: limit, select: { id: true, nodeId: true, capturedAt: true, receivedAt: true, latitude: true, longitude: true, batteryMv: true, temperature: true, humidity: true, pressure: true, rainfallMm: true, waterLevel: true, soilMoisture: true, windSpeed: true, vibration: true, mq2: true, mq135: true, turbidity: true, hazard: true, riskScore: true, prediction: true } });
    return rows.map((row) => ({ ...row, id: row.id.toString() }));
  }
  async findUserByUsername(username) {
    if (this.prisma) {
      try {
        return await this.prisma.user.findUnique({ where: { username }, select: { username: true, passwordHash: true, role: true, active: true } });
      } catch {
        // Fallback to local state if database fails
      }
    }
    return (this.state.users || []).find((u) => u.username === username) || null;
  }
  async createUser(username, passwordHash, role = "OPERATOR") {
    if (this.prisma) {
      try {
        return await this.prisma.user.create({ data: { username, passwordHash, role }, select: { username: true, role: true, active: true, createdAt: true } });
      } catch (error) {
        if (error.code === "P2002") throw error;
        // Fallback to local state if database fails
      }
    }
    if (!this.state.users) this.state.users = [];
    if (this.state.users.some((u) => u.username === username)) {
      const error = new Error("Username taken");
      error.code = "P2002";
      throw error;
    }
    const user = { username, passwordHash, role, active: true, createdAt: new Date().toISOString() };
    this.state.users.push(user);
    await this.persistJson();
    return { username: user.username, role: user.role, active: user.active, createdAt: user.createdAt };
  }
  async saveChat({ conversationId, username, question, answer, sources, provider }) {
    if (!this.prisma) return null;
    const id = conversationId || crypto.randomUUID();
    const existing = await this.prisma.chatConversation.findFirst({ where: { id, ownerUsername: username }, select: { id: true } });
    if (!existing) await this.prisma.chatConversation.create({ data: { id, ownerUsername: username, title: question.slice(0, 120) } });
    await this.prisma.$transaction([
      this.prisma.chatMessage.create({ data: { conversationId: id, role: "user", content: question } }),
      this.prisma.chatMessage.create({ data: { conversationId: id, role: "assistant", content: answer, sources: sources || undefined, provider: provider || undefined } }),
      this.prisma.chatConversation.update({ where: { id }, data: { updatedAt: new Date(), title: question.slice(0, 120) } }),
    ]);
    return id;
  }
  async listConversations(username, admin = false) {
    if (!this.prisma) return [];
    return this.prisma.chatConversation.findMany({ where: admin ? undefined : { ownerUsername: username }, orderBy: { updatedAt: "desc" }, take: 100, include: { _count: { select: { messages: true } } } });
  }
  async getConversation(id, username, admin = false) {
    if (!this.prisma) return null;
    return this.prisma.chatConversation.findFirst({ where: { id, ...(admin ? {} : { ownerUsername: username }) }, include: { messages: { orderBy: { createdAt: "asc" } } } });
  }
  async deleteConversation(id, username, admin = false) {
    if (!this.prisma) return false;
    const conversation = await this.prisma.chatConversation.findFirst({ where: { id, ...(admin ? {} : { ownerUsername: username }) }, select: { id: true } });
    if (!conversation) return false;
    await this.prisma.chatConversation.delete({ where: { id } });
    return true;
  }
  async listUsers() {
    if (this.prisma) {
      try {
        return await this.prisma.user.findMany({ orderBy: { createdAt: "desc" }, select: { username: true, role: true, active: true, createdAt: true } });
      } catch {}
    }
    return (this.state.users || []).map((u) => ({ username: u.username, role: u.role, active: u.active, createdAt: u.createdAt }));
  }
  async updateUser(username, changes) {
    if (this.prisma) {
      try {
        return await this.prisma.user.update({ where: { username }, data: changes, select: { username: true, role: true, active: true, createdAt: true } });
      } catch (error) {
        if (error.code === "P2025") throw error;
      }
    }
    const user = (this.state.users || []).find((u) => u.username === username);
    if (!user) {
      const error = new Error("User not found");
      error.code = "P2025";
      throw error;
    }
    if (changes.role !== undefined) user.role = changes.role;
    if (changes.active !== undefined) user.active = changes.active;
    await this.persistJson();
    return { username: user.username, role: user.role, active: user.active, createdAt: user.createdAt };
  }  async close() { await this.prisma?.$disconnect(); }
  addClient(client) { this.clients.add(client); client.on("close", () => this.clients.delete(client)); }
  broadcast(message) { const encoded = JSON.stringify(message); for (const client of this.clients) if (client.readyState === 1) client.send(encoded); }
  snapshot() { return { telemetry: this.state.telemetry.slice(0, 100), alerts: this.state.alerts.slice(0, 100), heartbeats: this.state.heartbeats.slice(0, 100), events: this.state.events.slice(0, 100) }; }
}
