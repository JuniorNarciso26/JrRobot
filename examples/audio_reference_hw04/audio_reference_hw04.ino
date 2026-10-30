// Isolated HW04 output reference. Arduino-ESP32 3.x, ESP32-S3 N16R8.
// This sketch replaces the JrBot application when flashed. It does not use Wi-Fi,
// microphone, WebRTC, or any JrBot code.
#include <Arduino.h>
#include <ESP_I2S.h>

constexpr int PIN_BCLK = 21;
constexpr int PIN_WS = 47;
constexpr int PIN_DOUT = 42;
constexpr int TONE_RATE = 8000;
constexpr int VOICE_RATE = 16000;
constexpr uint32_t MAX_VOICE_SAMPLES = VOICE_RATE * 10;

I2SClass i2s;

bool startI2s(int rate) {
  i2s.setPins(PIN_BCLK, PIN_WS, PIN_DOUT);
  return i2s.begin(I2S_MODE_STD, rate, I2S_DATA_BIT_WIDTH_16BIT,
                   I2S_SLOT_MODE_STEREO);
}

bool writeFrames(const int16_t *frames, size_t count) {
  const size_t bytes = count * 2 * sizeof(int16_t);
  return i2s.write(reinterpret_cast<const uint8_t *>(frames), bytes) == bytes;
}

void playReferenceTone(int amplitude) {
  if (!startI2s(TONE_RATE)) {
    Serial.println("ERROR tone_i2s_init");
    return;
  }
  // Exact waveform of the provided example: 8 kHz, 16-bit stereo,
  // square wave, amplitude 500. Integer half period gives about 444 Hz.
  constexpr unsigned halfPeriod = TONE_RATE / 440 / 2;
  int16_t sample = amplitude;
  int16_t frames[256 * 2];
  uint32_t index = 0;
  bool ok = true;
  for (int sent = 0; sent < TONE_RATE * 2; sent += 256) {
    const int framesInChunk = min(256, TONE_RATE * 2 - sent);
    for (int i = 0; i < framesInChunk; ++i, ++index) {
      if (index % halfPeriod == 0) sample = -sample;
      frames[2 * i] = sample;
      frames[2 * i + 1] = sample;
    }
    if (!writeFrames(frames, framesInChunk)) {
      ok = false;
      break;
    }
  }
  memset(frames, 0, sizeof(frames));
  if (!writeFrames(frames, 256)) ok = false;
  i2s.end();
  if (ok) {
    Serial.printf("TONE_DONE rate=8000 amplitude=%d\n", amplitude);
  } else {
    Serial.println("ERROR tone_write");
  }
}

void playVoice(uint32_t sampleCount) {
  if (sampleCount == 0 || sampleCount > MAX_VOICE_SAMPLES) {
    Serial.println("ERROR voice_length");
    return;
  }
  if (!startI2s(VOICE_RATE)) {
    Serial.println("ERROR voice_i2s_init");
    return;
  }
  uint8_t mono[512];
  int16_t stereo[256 * 2];
  uint32_t remaining = sampleCount;
  uint32_t peak = 0;
  uint32_t clipped = 0;
  Serial.println("VOICE_READY");
  while (remaining) {
    const size_t frames = min((uint32_t)256, remaining);
    const size_t need = frames * sizeof(int16_t);
    size_t received = 0;
    const uint32_t deadline = millis() + 5000;
    while (received < need && (int32_t)(deadline - millis()) > 0) {
      const size_t n = Serial.readBytes(mono + received, need - received);
      received += n;
    }
    if (received != need) {
      i2s.end();
      Serial.println("ERROR voice_serial_timeout");
      return;
    }
    for (size_t i = 0; i < frames; ++i) {
      const int16_t value = (int16_t)((uint16_t)mono[2 * i] |
                                      ((uint16_t)mono[2 * i + 1] << 8));
      const uint32_t magnitude = value == INT16_MIN ? 32768u : (uint32_t)abs(value);
      if (magnitude > peak) peak = magnitude;
      if (magnitude >= 32767u) ++clipped;
      stereo[2 * i] = value;
      stereo[2 * i + 1] = value;
    }
    if (!writeFrames(stereo, frames)) {
      i2s.end();
      Serial.println("ERROR voice_i2s_write");
      return;
    }
    remaining -= frames;
    Serial.println("CHUNK_OK");
  }
  memset(stereo, 0, sizeof(stereo));
  const bool tailOk = writeFrames(stereo, 256);
  i2s.end();
  if (tailOk) {
    Serial.printf("VOICE_DONE samples=%lu peak=%lu clipped=%lu\n",
                  (unsigned long)sampleCount, (unsigned long)peak,
                  (unsigned long)clipped);
  } else {
    Serial.println("ERROR voice_tail_write");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  delay(1200);
  Serial.println("HW04_AUDIO_REFERENCE tone=T loud=L voice=V_samples rate=16000 mono=pcm16le");
}

void loop() {
  if (!Serial.available()) return;
  char line[32];
  const size_t n = Serial.readBytesUntil('\n', line, sizeof(line) - 1);
  line[n] = '\0';
  if (n == 1 && (line[0] == 'T' || line[0] == 'L')) {
    playReferenceTone(line[0] == 'T' ? 500 : 5000);
  } else if (line[0] == 'V') {
    char *end = nullptr;
    const unsigned long count = strtoul(line + 1, &end, 10);
    if (end == line + 1 || *end != '\0' || count > MAX_VOICE_SAMPLES) {
      Serial.println("ERROR voice_length");
    } else {
      playVoice((uint32_t)count);
    }
  } else {
    Serial.println("ERROR command");
  }
}
