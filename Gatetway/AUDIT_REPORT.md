# Gateway Audit Report

**Date:** 2026-09-10

## Data Path

The gateway is now wired for this flow:

`LoRa packet -> PacketCodec decode -> header/payload validation -> AES-GCM authentication -> replay protection -> gateway routing -> payload decryption -> JSON conversion -> MQTT over TLS -> backend`

When Wi-Fi or MQTT is unavailable, decoded packets are retained in `GatewayQueue` and retried after reconnection.

## Fixed Problems

- Filled the previously empty gateway network headers by reusing the shared field-node protocol implementation.
- Corrected the gateway LoRa receive call to the shared `LoRaDriver` API.
- Added the missing shared `AckManager` include.
- Corrected `GatewayQueue` to use `GATEWAY_QUEUE_SIZE` and `QUEUE_MAX_PACKET_AGE_MS`.
- Added payload-type validation before dispatch.
- Authenticate encrypted packets before replay-cache insertion.
- Keep forwarded encrypted packets encrypted and do not mutate authenticated flags.
- Decrypt packets only after confirming they are addressed to the gateway.
- Added a matching gateway watchdog alias and corrected health statistics API usage.
- Made MQTT TLS fail closed when no root CA is configured.
- Synchronized the gateway development key with the field-node development key so local development packets can be decrypted.
- Added a dedicated `Gatetway/platformio.ini` with ESP32, LoRa, MQTT, and sensor-library build dependencies.

## Remaining Obvious Risks / Required Configuration

1. The gateway still uses a development AES key. Replace it through secure provisioning before deployment; never ship the key currently in source.
2. Wi-Fi credentials, MQTT broker, MQTT username/password, and MQTT root CA are intentionally absent. Provide them as private build definitions or secure provisioning values:
   - `SANKATNET_WIFI_SSID`
   - `SANKATNET_WIFI_PASSWORD`
   - `SANKATNET_MQTT_BROKER`
   - `SANKATNET_MQTT_USERNAME`
   - `SANKATNET_MQTT_PASSWORD`
   - `SANKATNET_MQTT_ROOT_CA`
3. `PubSubClient`'s basic publish API does not provide the configured QoS 1 behavior; the current `qos` argument is ignored. Use an MQTT client/library with QoS 1 support if delivery guarantees are required.
4. The gateway and field-node projects share protocol headers by relative include. Keep their packet structures, authentication rules, and protocol constants versioned together.
5. The timestamp replay policy still depends on node uptime values. Use synchronized time or a boot/session identity policy for production replay freshness.
6. `pio run` could not be executed in this environment because the `pio` executable is unavailable on PATH. Run it from a PlatformIO installation before flashing.
7. Validate SX1278 wiring, radio frequency/legal band, MQTT broker certificate chain, Wi-Fi credentials, and queue behavior on target hardware.

## File Completeness

All files listed in `Gatetway/filesys.txt` now contain either implementation code or a shared-protocol wrapper. The gateway-specific `platformio.ini` and this report were added for build/review support.
