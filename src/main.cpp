#include <Arduino.h>

namespace {
constexpr uint8_t kSoPin = 13;
constexpr uint8_t kCsPin = 10;
constexpr uint8_t kSckPin = 12;
constexpr uint32_t kSampleIntervalMs = 1000;
constexpr uint32_t kConversionTimeMs = 250;

uint32_t lastSampleMs = 0;
uint32_t conversionReadyMs = 0;
bool conversionStarted = false;

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
      Serial.printf("[ERROR] Lectura fuera de rango (uptime_ms=%lu)\n",
                    static_cast<unsigned long>(uptimeMs));
#if MAX6675_JSON_OUTPUT
      Serial.printf("{\"sensor\":\"MAX6675\",\"temperature_c\":null,\"status\":\"invalid_reading\",\"uptime_ms\":%lu}\n",
                    static_cast<unsigned long>(uptimeMs));
#endif
    } else {
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
  Serial.println("[INFO] Iniciando mediciones...");
  beginConversion();
  lastSampleMs = millis() - kSampleIntervalMs;
}

void loop() {
  const uint32_t now = millis();
  if (static_cast<int32_t>(now - lastSampleMs) >= static_cast<int32_t>(kSampleIntervalMs) &&
      conversionStarted && static_cast<int32_t>(now - conversionReadyMs) >= 0) {
    lastSampleMs = now;
    reportReading();
  }
  delay(1);
}
