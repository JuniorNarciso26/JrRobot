#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

namespace jr_yes_no {
constexpr int kRate = 16000;
constexpr int kBeepFrames = kRate * 120 / 1000;

// One response per detection burst, with at least 1.5 s between responses.
class BeepGate {
 public:
  bool accept(const char* word, float score, uint64_t now_ms) {
    const bool yes = std::strcmp(word, "yes") == 0 && score > 0.8f;
    if (!yes) {
      latched_ = false;
      return false;
    }
    if (latched_ || (has_beep_ && now_ms - last_ms_ < 1500)) return false;
    latched_ = has_beep_ = true;
    last_ms_ = now_ms;
    return true;
  }
 private:
  bool latched_ = false;
  bool has_beep_ = false;
  uint64_t last_ms_ = 0;
};

// Signed 32-bit I2S sample, 880 Hz at 12% amplitude, 5 ms attack/release.
inline int32_t beep_sample(int frame) {
  if (frame < 0 || frame >= kBeepFrames) return 0;
  constexpr int ramp = kRate * 5 / 1000;
  const int edge = frame < kBeepFrames - 1 - frame ? frame : kBeepFrames - 1 - frame;
  const float envelope = edge < ramp ? static_cast<float>(edge) / ramp : 1.0f;
  const float phase = 6.283185307179586f * 880.0f * frame / kRate;
  return static_cast<int32_t>(std::sin(phase) * envelope * 3932.0f) * 65536;
}
}  // namespace jr_yes_no
