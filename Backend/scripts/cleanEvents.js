import "dotenv/config";
import { PrismaClient } from "@prisma/client";

const prisma = new PrismaClient();
try {
  const deletedEvents = await prisma.record.deleteMany({
    where: {
      kind: "events",
      OR: [
        { payload: { path: ["message"], string_contains: "MQTT" } },
        { payload: { path: ["message"], string_contains: "reconnect" } }
      ]
    }
  });
  console.log(`Cleaned up ${deletedEvents.count} redundant MQTT event logs.`);

  const remainingRecords = await prisma.record.count();
  console.log(`Remaining total records in database: ${remainingRecords}`);
} finally {
  await prisma.$disconnect();
}
