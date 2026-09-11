# Sankat-Net Gateway

Sankat-Net Gateway is the bridge between the LoRa disaster-monitoring mesh
and the Internet/backend infrastructure.

The gateway receives authenticated packets from field nodes, validates them,
forwards mesh traffic when required, converts application payloads into
backend JSON, and publishes the data through MQTT over TLS.

---

## 1. Gateway Architecture

```text
                 FIELD NODE
                     │
                     │ SX1278 LoRa
                     ▼
              ┌───────────────┐
              │   LoRa Mesh   │
              │ NODE-01       │
              │ NODE-02       │
              │ NODE-03       │
              │ NODE-04       │
              └───────┬───────┘
                      │
                      ▼
              ┌───────────────┐
              │   GATEWAY     │
              │ ESP32         │
              │ SX1278        │
              └───────┬───────┘
                      │
                      │ Wi-Fi
                      ▼
              ┌───────────────┐
              │ MQTT Broker   │
              │ TLS : 8883    │
              └───────┬───────┘
                      │
                      ▼
              ┌───────────────┐
              │   Backend     │
              │ Node.js       │
              │ PostgreSQL    │
              └───────┬───────┘
                      │
                      │ WebSocket
                      ▼
              ┌───────────────┐
              │  Dashboard    │
              │ React + Map   │
              └───────────────┘
```
