# SankatNet2 Workspace Audit Report

**Audit date:** 2026-09-10  
**Scope:** All files currently present under the workspace, including `dataflow.txt`, `filesys.txt`, `packet.txt`, `main/`, `config/`, `sensors/`, `intelligence/`, `network/`, `storage/`, and `system/`.

## Executive Summary

The folder structure is present, but the firmware is not currently in a buildable or deployment-ready state. The most urgent problems are cross-file API mismatches that will prevent compilation, followed by incompatible packet encoding/decoding in the mesh path and security/replay risks that could cause communication failure or unsafe operation.

The review was source-based. A complete Arduino build could not be run because the workspace does not include the Arduino/ESP32 toolchain or library include paths.

## Findings

### Critical: guaranteed build blockers

1. **Incorrect watchdog class name.**
   - `main/SankatNet.ino` declares `WatchdogManager watchdog`.
   - `system/Watchdog.h` defines `class Watchdog`.
   - The sketch cannot compile until the type name is made consistent.

2. **Incorrect `FaultType` qualification.**
   - The sketch uses `FaultType::...`.
   - `FaultType` is declared inside `FaultManager` in `system/FaultManager.h`.
   - Use the correct qualification or move the enum to the intended namespace.

3. **Storage and battery method mismatches.**
   - The sketch calls `nvsManager.getBootCount()`, but the implementation provides `getBootCounter()`.
   - The sketch calls `packetQueue.empty()`, but the implementation provides `isEmpty()`.
   - The sketch expects `batteryManager.begin()` to return `bool`, while `BatteryManager::begin()` returns `void`.

4. **Initialization return-type mismatches.**
   - `MeshRouter::begin()` and `AckManager::begin()` return `void`, but the sketch tests both in `if (!...)` conditions.
   - `MeshRouter::expireRoutes()` requires a maximum-age argument, but the sketch calls it without one.

5. **LoRa configuration namespace and constant mismatches.**
   - `network/LoRaDriver.h` references constants such as `NodeConfig::LORA_FREQUENCY`, `LORA_BANDWIDTH`, `LORA_TX_POWER`, and `LORA_ENABLE_CRC`.
   - `config/NodeConfig.h` defines these under `LoRaConfig` with names such as `FREQUENCY`, `BANDWIDTH`, `TX_POWER_DBM`, and `ENABLE_CRC`.
   - `PacketCodec.h` references `NodeConfig::MAX_TTL` and `NodeConfig::MAX_HOPS`, which are not defined under those names.

6. **Sensor driver caller/API mismatches.**
   - `SensorManager.h` calls `bme280.begin(address)`, but `BME280Driver::begin()` takes no argument.
   - `SensorManager.h` calls `motion.begin(address)`, but the driver signature places the address second and expects a `TwoWire*` first; this call passes the address as the wrong type.
   - `SensorManager.h` calls `motion.read(ax, ay, az, magnitude)`, but `MotionDriver` only provides `read()` and exposes values through accessors.
   - `ADS1115Driver.h` calls calibration functions named `apply` and `convert`, while `Calibration.h` defines names such as `calibrate` and `normalize` for those namespaces.

7. **Duplicate enum definition.**
   - `SensorManager.h` and `Packet.h` both define `SankatNet::SensorStatusBits`.
   - Including both headers in the sketch produces a redefinition error.

8. **Missing sensor data field.**
   - `RiskEngine.h` reads `data.accelerationMagnitudeMS2`.
   - `SensorData` in `SensorManager.h` has acceleration axes and `vibrationMS2`, but no `accelerationMagnitudeMS2` member.

9. **Packet API mismatches in the main sketch.**
   - `PacketCodec::encode` requires four arguments, while the sketch calls it with three in the transmit path.
   - `LoRaDriver::receive` requires a `size_t& receivedLength` output argument, while the sketch calls it with two arguments.
   - `MeshRouter::getNextHop` returns a `uint16_t`, while the sketch uses an output-argument form.
   - `ReplayProtection::begin()` takes no arguments, while the sketch passes `NODE_ID`.
   - `HazardClassifier::classify` accepts a `RiskEngine::RiskResult` or sensor data plus engine; the sketch uses a call form that does not match the available overload.

10. **Alert payload mismatch.**
    - The sketch writes `fireRisk`, `floodRisk`, `seismicRisk`, and `pollutionRisk` into `AlertPayload`.
    - `AlertPayload` in `network/Packet.h` has no such fields.

### High: protocol and runtime correctness

1. **Mesh forwarding uses a different wire format from `PacketCodec`.**
   - `PacketCodec` serializes `header + payload + payloadLength + tag`.
   - `MeshRouter` forwarding serializes `header + payload + tag` and decodes using that alternate layout.
   - Packets encoded by one path are not reliably decodable by the other. There must be one shared codec implementation.

2. **Forwarding changes authenticated flags.**
   - `CryptoManager` authenticates the packet flags.
   - `MeshRouter` adds `FLAG_FORWARDED` while forwarding without recalculating the authentication tag.
   - A forwarded encrypted packet will fail end-to-end authentication. Either do not mutate authenticated fields or redesign the authentication/forwarding contract.

3. **ACK retries are never consumed by the main loop.**
   - `AckManager::update()` sets retry and delivery-failure state.
   - The sketch calls `ackManager.update()` but does not process `retryNeeded()` or `hasDeliveryFailure()`.
   - Retransmissions and delivery-failure handling therefore do not occur.

4. **ACK source is not validated.**
   - `AckManager::processAck()` matches only the acknowledged sequence number.
   - It does not verify that the ACK source is the expected destination for the pending packet.
   - A valid authenticated node could acknowledge another node's sequence if it knows the value.

5. **Replay protection is inconsistently integrated.**
   - `ReplayProtection` exposes `accept(header)` and `isDuplicate(source, sequence)`.
   - The sketch uses an incompatible `isDuplicate(packet.header)` form.
   - MeshRouter also has a separate duplicate cache, creating two independent replay/duplicate mechanisms that can disagree.

6. **Payload type validation is available but not enforced.**
   - `PacketCodec::validatePayloadType()` checks payload size against packet type.
   - The receive path does not consistently call it before interpreting decrypted payloads.
   - A structurally valid packet with an incorrect payload size can reach a handler.

7. **Alerts can be generated repeatedly without the configured interval.**
   - The risk loop can create alerts whenever the critical condition remains true.
   - `AlertConfig::ALERT_REPEAT_INTERVAL_MS` exists but is not consistently enforced in the sketch.
   - This can flood the queue and LoRa channel.

8. **Routing result is calculated but not enforced.**
   - The sketch computes a next hop, but transmission does not encode or otherwise enforce that selected hop.
   - The implementation falls back toward broadcast behavior rather than guaranteed route-directed delivery.

9. **Sensor manager reports initialized after partial failure.**
   - `SensorManager::begin()` records individual failures and returns `success`, but sets `initialized = true` unconditionally.
   - The main loop can continue sampling paths whose hardware did not initialize. This may be intentional degraded mode, but it should be explicit and each driver must be guarded consistently.

10. **Timestamp replay validation is unsafe across unsynchronized nodes.**
    - `ReplayProtection` compares packet uptime-style timestamps against the receiving node's `millis()` value.
    - Different boots and node uptimes are not synchronized, so valid packets may be rejected as expired or old packets may appear recent after a reboot.

### High: security and deployment risks

1. **Development AES key is compiled into firmware.**
   - `config/SecurityConfig.h` contains a static 32-byte network key.
   - `DEVELOPMENT_MODE` is enabled.
   - This is acceptable only for isolated development and must be replaced with a provisioning/key-storage design before deployment.

2. **NVS failure can lead to sequence `0`.**
   - `NVSManager::nextSequence()` returns `0` when NVS is not initialized.
   - The system readiness policy must refuse encrypted transmission when persistent sequence storage is unavailable; otherwise nonce/sequence identity can be reused.

3. **Nonce persistence and uniqueness need an explicit failure policy.**
   - The nonce combines node/sequence data with random bytes, but sequence persistence is not guaranteed when NVS fails.
   - Encryption must stop on NVS failure, and sequence writes should be checked for success before a packet is transmitted.

### Medium: maintainability and integration gaps

1. **The documented data flow is not represented by a build/test project.**
   - `dataflow.txt` describes a complete sensor-to-gateway pipeline, but there is no Arduino project configuration, dependency manifest, board configuration, or automated protocol test.

2. **The wire protocol documentation and implementation are incomplete.**
   - `packet.txt` describes the header, encrypted payload, and tag, but does not specify byte order, payload-length placement, exact payload sizes, or mutable/authenticated fields.
   - Those details must be written down and tested against `PacketCodec`.

3. **Multiple header-defined implementations increase linkage risk.**
   - Several classes and static data members are implemented directly in headers.
   - `WeatherDriver` uses an inline static variable, which is appropriate for C++17, but the project must explicitly compile with a compatible C++ standard.

4. **Hardware assumptions are not yet validated.**
   - Pin assignments, LoRa frequency, interrupt availability, sensor addresses, and active-low flame behavior are configured as assumptions.
   - These require hardware-in-the-loop validation before field use.

## Recommended Repair Order

1. Make the project compile by resolving all class, namespace, method-signature, and field mismatches.
2. Select one packet wire format and make both transmit and receive paths use `PacketCodec` exclusively.
3. Define the authentication contract for forwarding. Do not mutate authenticated flags without a valid tag strategy.
4. Integrate replay acceptance, payload-type validation, ACK source validation, retries, and delivery failures into one receive/transmit flow.
5. Block encrypted transmission when NVS or sequence persistence fails.
6. Replace the development key and disable development mode for deployment builds.
7. Add an Arduino/ESP32 build configuration and host-side tests for packet encoding, decoding, encryption, replay, routing, and ACK behavior.
8. Validate all pin, address, interrupt, radio, and sensor assumptions on the target hardware.

## Verification Limits

A full compiler or hardware test was not possible in this workspace because Arduino/ESP32 headers and third-party libraries such as `Adafruit_BME280`, `Adafruit_ADS1X15`, `Adafruit_MPU6050`, `TinyGPSPlus`, `LoRa`, `OneWire`, and `DallasTemperature` are not configured in the analyzer. The compile blockers listed above are nevertheless visible from the source-level API mismatches.

## Remediation Applied

The following source-level fixes were applied after the audit:

- Added `platformio.ini` for an ESP32 DevKit build with C++17 and the declared sensor, GPS, LoRa, OneWire, and DallasTemperature dependencies.
- Corrected the watchdog, fault, NVS, battery, mesh-router, ACK-manager, sensor-driver, calibration, and LoRa configuration API mismatches.
- Added the missing alert risk fields and removed the duplicate sensor-status enum type collision.
- Replaced the mesh router's private wire format with the canonical `PacketCodec` format, including the payload-length field.
- Added non-mutating packet authentication so encrypted packets can be authenticated, replay-checked, routed, and only decrypted at their destination.
- Stopped mesh forwarding from mutating authenticated flags.
- Added payload-type validation before dispatch, ACK-source validation, alert repeat throttling, and persistent sequence-allocation failure handling.
- Added ACK retry packet ownership and a main-loop retry consumer.
- Prevented NVS sequence wraparound and blocked packet creation when persistent sequence allocation fails.
- Added the missing mesh destination check, removed duplicate gateway/broadcast constants, and corrected all remaining battery API calls.

## Remaining Production Gates

This workspace is improved but must not yet be described as field-ready until these gates pass:

1. Run `pio run` with PlatformIO and resolve any library-version or ESP32-core-specific diagnostics. The current environment does not have the `pio` executable installed, so this build was not run here.
2. Replace the compiled development AES key with a provisioning/key-storage mechanism and set `DEVELOPMENT_MODE` appropriately for the deployment build.
3. Define and test a synchronized time or boot/session policy if timestamp freshness rejection is required; the current uptime values are not synchronized between nodes.
4. Add host-side tests for packet round trips, GCM authentication, forwarding, replay, ACK retries, and malformed payload rejection.
5. Verify the configured pins, I2C addresses, LoRa frequency/legal band, interrupt support, sensor polarity, and watchdog behavior on the target hardware.
