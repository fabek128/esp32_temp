#include <Arduino.h>
#include <WiFi.h>

namespace {
constexpr uint8_t kSoPin = 13;
constexpr uint8_t kCsPin = 10;
constexpr uint8_t kSckPin = 12;
// GPIO4/5 are standard ESP32-S3 GPIOs and do not overlap the MAX6675 pins.
// Confirm the board exposes these pads before wiring.
constexpr uint8_t kFanPwmPin = 4;
constexpr uint8_t kFanTachPin = 5;
constexpr uint32_t kSampleIntervalMs = 1000;
constexpr uint32_t kConversionTimeMs = 250;
constexpr uint32_t kWifiRetryIntervalMs = 10000;
constexpr uint32_t kFanPwmFrequencyHz = 25000;
constexpr uint32_t kFanTachStartupGraceMs = 3000;
constexpr uint8_t kFanPwmResolutionBits = 10;
constexpr uint8_t kFanPwmChannel = 0;
constexpr uint16_t kFanPwmMax = (1U << kFanPwmResolutionBits) - 1U;
constexpr uint8_t kFanMinDutyPercent = 40;
constexpr float kFanCurveStartC = 50.0f;
constexpr float kFanCurveFullC = 80.0f;
constexpr uint8_t kFanCurveStartDutyPercent = 40;

volatile uint32_t tachPulseCount = 0;
uint32_t lastRpmReportMs = 0;
uint32_t fanControllerStartedMs = 0;
uint8_t currentFanDutyPercent = 100;
bool fanPwmReady = false;
bool tachFaultLatched = false;

void IRAM_ATTR onFanTachPulse() {
  ++tachPulseCount;
}

uint8_t fanDutyForTemperature(float celsius) {
  if (celsius <= kFanCurveStartC) {
    return kFanMinDutyPercent;
  }
  if (celsius >= kFanCurveFullC) {
    return 100;
  }
  const float fraction = (celsius - kFanCurveStartC) /
                         (kFanCurveFullC - kFanCurveStartC);
  const float duty = kFanCurveStartDutyPercent +
                     fraction * (100 - kFanCurveStartDutyPercent);
  return static_cast<uint8_t>(duty + 0.5f);
}

void setFanDuty(uint8_t dutyPercent) {
  if (!fanPwmReady) {
    return;
  }
  if (dutyPercent == 0) {
    // Shutdown is intentionally disabled: zero requests map to the safe floor.
    dutyPercent = kFanMinDutyPercent;
  }
  dutyPercent = constrain(dutyPercent, kFanMinDutyPercent, 100);
  currentFanDutyPercent = dutyPercent;

  // The NPN stage inverts the LEDC waveform: a high GPIO turns the NPN on
  // and pulls the fan's PWM pin low. Therefore the GPIO duty is inverted.
  const uint32_t gpioDuty =
      static_cast<uint32_t>(kFanPwmMax) * (100U - dutyPercent) / 100U;
  ledcWrite(kFanPwmChannel, gpioDuty);
}

void reportFanRpm() {
  const uint32_t now = millis();
  if (now - lastRpmReportMs < 1000) {
    return;
  }
  lastRpmReportMs = now;
  noInterrupts();
  const uint32_t pulses = tachPulseCount;
  tachPulseCount = 0;
  interrupts();
  // Intel 4-wire fan interface uses two tach pulses per revolution.
  const uint32_t rpm = pulses * 30U;
  if (pulses == 0 && !tachFaultLatched &&
      now - fanControllerStartedMs >= kFanTachStartupGraceMs) {
    tachFaultLatched = true;
    setFanDuty(100);
    Serial.println("[FAN][ERROR] sin pulsos de tacometro; maxima velocidad enclavada");
  }
  Serial.printf("[FAN] duty=%u%% rpm=%lu\n", currentFanDutyPercent,
                static_cast<unsigned long>(rpm));
}

uint32_t lastSampleMs = 0;
uint32_t conversionReadyMs = 0;
bool conversionStarted = false;
uint32_t lastWifiAttemptMs = 0;
bool wifiStatusInitialized = false;
bool wifiWasConnected = false;
int lastWifiStatus = -1;

void updateWifi() {
#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
  const uint32_t now = millis();
  const wl_status_t wifiStatus = WiFi.status();
  const bool connected = wifiStatus == WL_CONNECTED;
  if (static_cast<int>(wifiStatus) != lastWifiStatus) {
    lastWifiStatus = static_cast<int>(wifiStatus);
    Serial.printf("[WIFI] estado=%d\n", lastWifiStatus);
  }
  if (connected != wifiWasConnected || !wifiStatusInitialized) {
    wifiWasConnected = connected;
    wifiStatusInitialized = true;
    Serial.println(connected ? "[WIFI] conectado" : "[WIFI] desconectado; reintento automatico");
  }
  if (!connected && (lastWifiAttemptMs == 0 || now - lastWifiAttemptMs >= kWifiRetryIntervalMs)) {
    lastWifiAttemptMs = now;
    if (WiFi.getMode() == WIFI_OFF) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.println("[WIFI] conexion iniciada");
    } else {
      WiFi.reconnect();
    }
  }
#endif
}

uint16_t readMax6675Frame() {
  uint16_t frame = 0;
  digitalWrite(kCsPin, LOW);
  delayMicroseconds(1);
  for (uint8_t bit = 0; bit < 16; ++bit) {
    digitalWrite(kSckPin, HIGH);
    frame = static_cast<uint16_t>((frame << 1) | (digitalRead(kSoPin) ? 1 : 0));
    digitalWrite(kSckPin, LOW);
  }
  digitalWrite(kCsPin, HIGH);
  return frame;
}

void beginConversion() {
  // MAX6675 converts while CS is high; leave it idle for the full conversion window.
  digitalWrite(kCsPin, HIGH);
  conversionReadyMs = millis() + kConversionTimeMs;
  conversionStarted = true;
}

void reportReading() {
  const uint16_t frame = readMax6675Frame();
  const uint32_t uptimeMs = millis();
  if ((frame & 0x4) != 0) {
    setFanDuty(100);
    Serial.println("[FAN] sensor fault; commanded full speed");
    Serial.printf("[ERROR] Thermocouple desconectada (status=open, uptime_ms=%lu)\n",
                  static_cast<unsigned long>(uptimeMs));
#if MAX6675_JSON_OUTPUT
    Serial.printf("{\"sensor\":\"MAX6675\",\"temperature_c\":null,\"status\":\"thermocouple_disconnected\",\"uptime_ms\":%lu}\n",
                  static_cast<unsigned long>(uptimeMs));
#endif
  } else {
    const uint16_t raw = static_cast<uint16_t>((frame >> 3) & 0x0FFF);
    const float celsius = raw * 0.25f;
    if (celsius > 1024.0f) {
      setFanDuty(100);
      Serial.println("[FAN] invalid temperature; commanded full speed");
      Serial.printf("[ERROR] Lectura fuera de rango (uptime_ms=%lu)\n",
                    static_cast<unsigned long>(uptimeMs));
#if MAX6675_JSON_OUTPUT
      Serial.printf("{\"sensor\":\"MAX6675\",\"temperature_c\":null,\"status\":\"invalid_reading\",\"uptime_ms\":%lu}\n",
                    static_cast<unsigned long>(uptimeMs));
#endif
    } else {
      setFanDuty(tachFaultLatched ? 100 : fanDutyForTemperature(celsius));
      Serial.printf("[TEMP] %.2f C\n", celsius);
#if MAX6675_JSON_OUTPUT
      Serial.printf("{\"sensor\":\"MAX6675\",\"temperature_c\":%.2f,\"status\":\"ok\",\"uptime_ms\":%lu}\n",
                    celsius, static_cast<unsigned long>(uptimeMs));
#endif
    }
  }
  beginConversion();
}
}  // namespace

void setup() {
  fanControllerStartedMs = millis();
  // Keep the external NPN off during reset/startup; an open PWM line requests
  // full fan speed. The external 10k base pull-down provides the same default
  // while the ESP32 GPIO is high impedance.
  pinMode(kFanPwmPin, OUTPUT);
  digitalWrite(kFanPwmPin, LOW);
  fanPwmReady = ledcSetup(kFanPwmChannel, kFanPwmFrequencyHz,
                          kFanPwmResolutionBits) != 0;
  if (fanPwmReady) {
    ledcAttachPin(kFanPwmPin, kFanPwmChannel);
    setFanDuty(100);
  }

  pinMode(kFanTachPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(kFanTachPin), onFanTachPulse, FALLING);

  pinMode(kSoPin, INPUT);
  pinMode(kCsPin, OUTPUT);
  pinMode(kSckPin, OUTPUT);
  digitalWrite(kSckPin, LOW);
  digitalWrite(kCsPin, HIGH);

  Serial.begin(115200);
  // Arduino USB CDC can be disabled by the board configuration. The ESP32-S3
  // native USB Serial/JTAG port remains available as Serial0 in that case.
#if !ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(115200);
#endif
  const uint32_t serialWaitStarted = millis();
  while (!Serial && millis() - serialWaitStarted < 2000) {
    delay(10);
  }
  Serial.println("[INIT] ESP32-S3 iniciado");
  Serial.println("[INIT] MAX6675 configurado");
  Serial.println("[INIT] SO=13 CS=10 SCK=12");
  Serial.printf("[FAN] PWM GPIO=%u tach GPIO=%u PWM=%lu Hz state=%s\n",
                kFanPwmPin, kFanTachPin,
                static_cast<unsigned long>(kFanPwmFrequencyHz),
                fanPwmReady ? "ready/full-speed-default" : "unavailable");
#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttemptMs = millis();
  Serial.println("[WIFI] conexion iniciada");
#else
  Serial.println("[WIFI] credenciales no configuradas");
#endif
  Serial.println("[INFO] Iniciando mediciones...");
  beginConversion();
  lastSampleMs = millis() - kSampleIntervalMs;
}

void loop() {
  updateWifi();
  reportFanRpm();
  const uint32_t now = millis();
  if (static_cast<int32_t>(now - lastSampleMs) >= static_cast<int32_t>(kSampleIntervalMs) &&
      conversionStarted && static_cast<int32_t>(now - conversionReadyMs) >= 0) {
    lastSampleMs = now;
    reportReading();
  }
  delay(1);
}
