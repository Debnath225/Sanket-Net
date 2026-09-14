import "dotenv/config";
import { PrismaClient } from "@prisma/client";

if (!process.env.DATABASE_URL) throw new Error("DATABASE_URL is required.");
const prisma = new PrismaClient();
try {
  const readings = await prisma.sensorReading.deleteMany({ where: { nodeId: { startsWith: "DEMO-" } } });
  const records = await prisma.record.deleteMany({ where: { kind: "telemetry", payload: { path: ["node_id"], string_starts_with: "DEMO-" } } });
  console.log(`Removed ${readings.count} demo sensor readings and ${records.count} demo telemetry records.`);
} finally { await prisma.$disconnect(); }
