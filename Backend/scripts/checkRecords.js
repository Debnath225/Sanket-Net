import "dotenv/config";
import { PrismaClient } from "@prisma/client";

const prisma = new PrismaClient();
try {
  const records = await prisma.record.findMany({
    where: {
      OR: [
        { payload: { path: ["node_id"], string_starts_with: "DEMO-" } },
        { payload: { path: ["nodeId"], string_starts_with: "DEMO-" } },
        { payload: { path: ["source_node"], string_starts_with: "DEMO-" } }
      ]
    },
    take: 20
  });
  console.log("Demo records found:", records.length);
  for (const r of records) {
    console.log(r.id, r.kind, r.payload?.node_id || r.payload?.nodeId);
  }

  const allRecords = await prisma.record.findMany({
    take: 10,
    orderBy: { createdAt: "desc" }
  });
  console.log("Total recent records:", allRecords.length);
  for (const r of allRecords) {
    console.log("Record:", r.id, r.kind, r.payload?.node_id || r.payload?.message);
  }
} finally {
  await prisma.$disconnect();
}
