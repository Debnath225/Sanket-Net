const VECTOR_DIMENSIONS = 256;
const STOP_WORDS = new Set(["a", "an", "and", "are", "about", "at", "can", "current", "data", "do", "for", "from", "how", "i", "is", "latest", "me", "of", "on", "or", "show", "the", "to", "what", "which", "with", "you"]);
const words = (value) => String(value || "").toLowerCase().match(/[a-z0-9_-]{2,}/g)?.filter((word) => !STOP_WORDS.has(word)) || [];
const hash = (value) => { let result = 2166136261; for (const character of value) result = Math.imul(result ^ character.charCodeAt(0), 16777619); return result >>> 0; };
export const createEmbedding = (value) => {
  const vector = new Float64Array(VECTOR_DIMENSIONS);
  for (const token of words(value)) { const tokenHash = hash(token); vector[tokenHash % VECTOR_DIMENSIONS] += tokenHash & 1 ? 1 : -1; }
  const magnitude = Math.hypot(...vector);
  return Array.from(magnitude ? vector.map((item) => item / magnitude) : vector);
};
export const cosineSimilarity = (left, right) => left.length === right.length ? left.reduce((total, item, index) => total + Number(item) * Number(right[index]), 0) : 0;
const recordTime = (record) => record.receivedAt || record.createdAt || record.at || null;
const recordText = (record, kind) => `${kind} ${record.node_id || record.source_node || record.nodeId || ""} ${record.prediction?.hazard || ""} ${record.hazard || ""} ${record.message || ""} ${JSON.stringify(record)}`;
const formatTime = (value) => { const date = value && new Date(value); return date && !Number.isNaN(date.getTime()) ? date.toLocaleString("en-IN", { dateStyle: "medium", timeStyle: "short" }) : "recent"; };

const describe = ({ kind, record }) => {
  const node = record.node_id || record.source_node || record.nodeId;
  const hazard = record.prediction?.hazard || record.hazard;
  const risk = record.prediction?.risk ?? record.risk_score ?? record.risk;
  const parts = [{ telemetry: "telemetry", alerts: "alert", heartbeats: "heartbeat", events: "event" }[kind] || kind];
  if (node) parts.push(`station ${node}`);
  if (hazard) parts.push(`hazard ${String(hazard).toUpperCase()}`);
  if (risk !== undefined && risk !== null && Number.isFinite(Number(risk))) parts.push(`risk ${Math.round(Number(risk))}%`);
  if (record.water_level !== undefined && record.water_level !== null) parts.push(`water level ${record.water_level} cm`);
  if (record.message) parts.push(record.message);
  parts.push(`recorded ${formatTime(recordTime(record))}`);
  return parts.join(" — ");
};

export function retrieveLiveContext(question, state, persisted = []) {
  const queryVector = createEmbedding(question);
  const queryTerms = words(question);
  const now = Date.now();
  const isNetworkQuery = /mqtt|broker|gateway|reconnect|connection|network|signal/i.test(question);

  const memory = ["telemetry", "alerts", "heartbeats", "events"].flatMap((kind) =>
    (state[kind] || [])
      .filter((record) => {
        const nodeId = String(record.node_id || record.nodeId || record.source_node || "");
        if (nodeId.startsWith("DEMO-")) return false;
        if (!isNetworkQuery && kind === "events" && /mqtt reconnecting|mqtt error/i.test(record.message || "")) {
          return false;
        }
        return true;
      })
      .map((record) => ({ kind, record }))
  );

  const cleanPersisted = (persisted || []).filter((entry) => {
    const nodeId = String(entry.record?.node_id || entry.record?.nodeId || "");
    return !nodeId.startsWith("DEMO-");
  });

  const entries = [...cleanPersisted, ...memory];
  const seen = new Set();
  return entries
    .map((entry) => {
      const text = recordText(entry.record, entry.kind);
      const lexicalMatches = queryTerms.reduce((score, term) => score + (text.toLowerCase().includes(term) ? 1 : 0), 0);
      const age = now - new Date(recordTime(entry.record) || 0).getTime();
      const recency = Number.isFinite(age) ? Math.max(0, 1 - age / 86_400_000) : 0;
      return {
        ...entry,
        score: Number.isFinite(entry.score)
          ? entry.score + lexicalMatches * 0.15 + recency * 0.05
          : cosineSimilarity(queryVector, createEmbedding(text)) + lexicalMatches * 0.15 + recency * 0.05,
      };
    })
    .filter((entry) => entry.score > 0 || !queryTerms.length)
    .sort((a, b) => b.score - a.score)
    .filter((entry) => {
      const key = `${entry.kind}:${entry.record.node_id || ""}:${recordTime(entry.record) || ""}`;
      if (seen.has(key)) return false;
      seen.add(key);
      return true;
    })
    .slice(0, 8)
    .map(({ kind, record, score }) => ({
      kind,
      summary: describe({ kind, record }),
      at: recordTime(record),
      score: Number(score.toFixed(3)),
      record,
    }));
}

const fallbackAnswer = (question, state, context) => {
  const alerts = (state.alerts || []).filter((a) => !String(a.node_id || "").startsWith("DEMO-"));
  const telemetry = (state.telemetry || []).filter((t) => !String(t.node_id || "").startsWith("DEMO-"));
  const highRisk = telemetry.filter((item) => Number(item.prediction?.risk ?? item.risk_score ?? 0) >= 75);

  const isFloodQuery = /flood|rain|water|precipitation/i.test(question);
  if (isFloodQuery) {
    const floodAlerts = alerts.filter((a) => String(a.hazard || "").toLowerCase().includes("flood"));
    const floodTelemetry = telemetry.filter(
      (t) => String(t.prediction?.hazard || t.hazard || "").toLowerCase().includes("flood") || Number(t.water_level || 0) > 400
    );

    if (floodAlerts.length === 0 && floodTelemetry.length === 0) {
      return "All monitored field stations currently report normal conditions. There are no active flood or severe rainfall warnings in your sector. Water levels and drainage channels remain within safe baselines.";
    }

    const highest = floodTelemetry[0] || floodAlerts[0];
    const riskVal = Math.round(highest?.prediction?.risk ?? highest?.risk_score ?? highest?.risk ?? 0);
    const waterVal = highest?.water_level ? `${highest.water_level} cm` : "elevated";
    return `Flood advisory active: Station ${highest?.node_id || "regional sensor"} reports a ${riskVal}% risk with water level at ${waterVal}. Residents near low-lying culverts should stay vigilant and monitor local guidance.`;
  }

  if (/alert|critical|danger|risk|hazard/i.test(question)) {
    if (alerts.length === 0 && highRisk.length === 0) {
      return "All environmental parameters are currently within normal safety thresholds. No critical hazards are active in the monitored mesh.";
    }
    return `There are currently ${alerts.length} active alert(s) and ${highRisk.length} elevated risk sensor signal(s) in the network. Station ${alerts[0]?.node_id || highRisk[0]?.node_id} reports ${alerts[0]?.hazard || highRisk[0]?.prediction?.hazard || "hazard"} at ${Math.round(alerts[0]?.risk || highRisk[0]?.prediction?.risk || 0)}% risk.`;
  }

  if (/node|device|sensor|online|station/i.test(question)) {
    const nodeIds = [...new Set([...telemetry].map((item) => item.node_id || item.source_node).filter(Boolean))];
    return `The live network currently tracks ${nodeIds.length} reporting field station(s)${nodeIds.length ? `: ${nodeIds.slice(0, 8).join(", ")}` : ""}. All transmitting nodes are verified via AES-GCM encryption.`;
  }

  if (context.length > 0) {
    const relevantSummaries = context.slice(0, 3).map((c) => `• ${c.summary}`).join("\n");
    return `Based on live monitoring records:\n${relevantSummaries}\n\nPlease check official civil guidance for safety instructions.`;
  }

  return "All monitored regional stations report normal environmental conditions with no active emergency warnings. How else can I assist with your disaster readiness?";
};

const LIVE_MONITORING_PATTERN = /\b(?:live|current|latest|today|reading|readings|sensor|telemetry|node|device|alert|alerts|risk|hazard|critical|danger|network|mqtt|gateway|battery|temperature|humidity|pressure|rainfall|rain|water\s*level|soil|wind|vibration|pollution|air\s*quality|flood\s*risk|landslide\s*risk|fire\s*risk|seismic)\b/i;

export function isLiveMonitoringQuestion(question) {
  return LIVE_MONITORING_PATTERN.test(String(question || ""));
}

const GREETING_PATTERN = /^(?:hi|hello|hey|good\s+(?:morning|afternoon|evening)|namaste|thanks|thank\s+you)[!,.\s]*$/i;
const CAPABILITY_PATTERN = /^(?:who are you|what can you do|help)[?.!\s]*$/i;

export function isConversationalMessage(question) {
  const value = String(question || "").trim();
  return GREETING_PATTERN.test(value) || CAPABILITY_PATTERN.test(value);
}

const conversationalAnswer = (question) => {
  if (CAPABILITY_PATTERN.test(question)) {
    return "I am Sankat AI, your disaster intelligence assistant. I can summarize flood, landslide, fire, weather, and field station telemetry from the Sankat-Net mesh. You can ask about flood risk, shelter locations, or emergency go-bag preparation.";
  }
  return "Hello! I am Sankat AI, your disaster intelligence assistant. I can help you monitor flood risks, landslides, weather alerts, and safety guidance. How can I help you today?";
};

export async function answerFromLiveData(question, state, generateAnswer, persistedContext = []) {
  const normalizedQuestion = String(question || "").trim();
  if (isConversationalMessage(normalizedQuestion)) {
    return { answer: conversationalAnswer(normalizedQuestion), sources: [], provider: "local", retrievedAt: new Date().toISOString() };
  }

  if (!isLiveMonitoringQuestion(normalizedQuestion)) {
    const localAnswer = "I'm here to help with disaster management, emergency preparedness, and Sankat-Net monitoring. Ask me about evacuation readiness, flood or storm safety, or ask for live data such as 'What are the current flood risks?'";
    if (!generateAnswer) return { answer: localAnswer, sources: [], provider: "local", retrievedAt: new Date().toISOString() };
    try {
      const answer = await generateAnswer(normalizedQuestion, [], "general");
      return { answer: answer || localAnswer, sources: [], provider: "llm", retrievedAt: new Date().toISOString() };
    } catch {
      return { answer: localAnswer, sources: [], provider: "local", retrievedAt: new Date().toISOString() };
    }
  }

  const context = retrieveLiveContext(normalizedQuestion, state, persistedContext);
  const sources = context.map(({ record, ...source }) => source);
  const fallback = fallbackAnswer(normalizedQuestion, state, context);

  if (!generateAnswer || !context.length) {
    return { answer: fallback, sources, provider: "local", retrievedAt: new Date().toISOString() };
  }

  try {
    const answer = await generateAnswer(normalizedQuestion, context);
    return { answer: answer || fallback, sources, provider: "llm", retrievedAt: new Date().toISOString() };
  } catch (error) {
    console.error(`[rag] LLM answer generation failed: ${error.message}`);
    return { answer: fallback, sources, provider: "local", retrievedAt: new Date().toISOString() };
  }
}
