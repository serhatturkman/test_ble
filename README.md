# test_ble

📝 Final Demo Implementation Plan
Gateway (BLE Central - GATT Client)
✅ Responsibilities:

Receives data from nodes:
BLE Notifications for periodic and alarm data.
Logs all received data (with RTC timestamp) to console.
Sends configuration updates:
Maintains a static 100-byte config array.
Every 5 minutes, modifies the config and sends the update.
Node (BLE Peripheral - GATT Server)
✅ Responsibilities:

Time Windows (MTW, DTW, ATW)

MTW (Measurement Time Window):

Period: 30 sec.
Awake Time: 2 sec (normal) / 5 sec (abnormal).
Reads mocked sensor data:
Accelerometer (FFT Input) → 2KB.
Microphone (PDM) → 400B.
ADC (12-bit) → 500B.
FFT analysis checks for abnormality.
If abnormal → Triggers AMTW.
DTW (Data Sending Time Window):

Period: 180 sec.
Awake Time: 5 sec max.
Sends last normal measurements via BLE Notification.
Fragmentation is required (MTU limit of ~244 bytes).
ATW (Alarm Sending Time Window):

Period: Random 100 - 500 sec.
Awake Time: 5 sec max.
Simulates an alarm trigger (e.g., motion detection).
Configuration Updates from Gateway

Waits for config updates after sending sensor data.
Uses BLE GATT Write to receive 100-byte config.
Mocking Sensor Data
Accelerometer (FFT Input - 2KB)

Will contain randomized frequency data.
FFT detects a configurable frequency threshold spike.
If an anomaly is found → Triggers AMTW.
Microphone (PDM - 400B)

Static or randomized data.
ADC (12-bit - 500B)

Static or randomized data.
✅ Configuration Thresholds

Default FFT anomaly threshold: 5000 (mocked).
Gateway can update this threshold via config updates.
BLE Data Transfer Strategy
DTW (Periodic Data Transfer)

BLE Notification sends 2KB + 400B + 500B = ~3KB total.
Data is fragmented (~244 bytes per packet).
AMTW (Immediate Alarm Data)

Same fragmentation process.
Sent as a priority notification.
Configuration Updates

GATT Write from Gateway sends 100 bytes.
Logging Format
To make the demo cool, we’ll log everything with RTC timestamps:

less
Copy
Edit
[1653423450ms] [Node 01] Wakeup: MTW started
[1653423452ms] [Node 01] FFT analysis: Normal
[1653423455ms] [Node 01] Sleeping...
[1653425250ms] [Node 01] Wakeup: DTW started
[1653425252ms] [Node 01] Sending Sensor Data (Fragment 1/13)...
[1653425255ms] [Node 01] Sleeping...
[1653426300ms] [Node 01] Wakeup: Alarm Triggered! Sending AMTW...
[1653426305ms] [Node 01] Sleeping...
[1653429000ms] [Gateway] Received Node 01 data (Fragmented 13/13)
[1653429001ms] [Gateway] Sent Configuration Update to Node 01
✅ RTC timestamps (in milliseconds) will be used for all logs.

Final Agreement ✅
Feature	Status
Gateway logs all received data via UART	✅
Config updates every 5 minutes	✅
Nodes have 3 time windows (MTW, DTW, ATW)	✅
MTW reads sensor data (mocked) + FFT check	✅
DTW sends last normal measurements via BLE	✅
ATW triggers alarm randomly (100 - 500 sec)	✅
FFT abnormality detection (configurable threshold)	✅
Fragmented data transfer (2KB, 400B, 500B)	✅
RTC timestamps for logs	✅
