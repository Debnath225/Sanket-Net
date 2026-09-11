const clamp = (value) => Math.max(0, Math.min(100, Math.round(value)));
const number = (value, fallback = 0) =>
  Number.isFinite(Number(value)) ? Number(value) : fallback;

export function predict(input) {
  const temperature = number(input.temperature);
  const humidity = number(input.humidity);
  const rainfall = number(input.rainfall_mm ?? input.rainfallMM);
  const water = number(input.water_level ?? input.waterLevel);
  const soil = number(input.soil_moisture ?? input.soilMoisture);
  const wind = number(input.wind_speed ?? input.windSpeed);
  const mq2 = number(input.mq2);
  const mq135 = number(input.mq135);
  const vibration = number(input.vibration);

  const fire = clamp(
    (temperature - 32) * 1.4 + (30 - humidity) * 0.8 + mq2 / 90 + mq135 / 120,
  );
  const flood = clamp(
    water / 10 + rainfall * 0.55 + Math.max(0, soil - 60) * 0.8,
  );
  const landslide = clamp(
    Math.max(0, soil - 65) * 1.4 +
      rainfall * 0.8 +
      Math.max(0, vibration - 2) * 12,
  );
  const forestFire = clamp(fire * 0.75 + Math.max(0, wind - 8) * 2);
  const seismic = clamp(Math.max(0, vibration - 0.8) * 18);
  const pollution = clamp(mq2 / 55 + mq135 / 55 + number(input.turbidity) / 15);

  const risks = { fire, flood, landslide, forestFire, seismic, pollution };
  const [hazard, risk] = Object.entries(risks).sort((a, b) => b[1] - a[1])[0];
  const confidence = clamp(45 + Math.abs(risk - 50) * 0.8);

  return {
    model: "explainable-threshold-v1",
    horizonMinutes: 30,
    hazard,
    risk,
    confidence,
    risks,
    recommendation: recommendationFor(hazard, risk),
    generatedAt: new Date().toISOString(),
  };
}

function recommendationFor(hazard, risk) {
  if (risk < 40) return "Continue monitoring";
  if (hazard === "flood") return "Inspect water level and downstream drainage";
  if (hazard === "landslide") return "Issue slope inspection warning";
  if (hazard === "forestFire" || hazard === "fire")
    return "Verify smoke/fire signal and alert local response";
  if (hazard === "seismic")
    return "Check vibration source and structural safety";
  if (hazard === "pollution")
    return "Sample air/water quality and inspect source";
  return "Increase observation frequency";
}
