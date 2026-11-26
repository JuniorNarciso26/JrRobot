"""Execute the actual portable C++ tone/gate used by the ESP firmware."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(shutil.which("g++"), "C++ compiler required")
class YesBeepTest(unittest.TestCase):
    def test_tone_and_detection_bursts(self):
        source = r'''
#include "yes_beep.h"
#include <cassert>
#include <cstdlib>
int main() {
  jr_yes_no::BeepGate gate;
  assert(!gate.accept("no", 0.99f, 0));
  assert(!gate.accept("unknown", 0.99f, 1));
  assert(!gate.accept("silence", 0.99f, 2));
  assert(!gate.accept("yes", 0.8f, 3));
  assert(gate.accept("yes", 0.99f, 100));
  assert(!gate.accept("yes", 0.99f, 500));
  assert(!gate.accept("yes", 0.99f, 3000)); // sustained detection stays latched
  assert(!gate.accept("unknown", 0.99f, 3100));
  assert(gate.accept("yes", 0.99f, 3200));
  assert(!gate.accept("unknown", 0.99f, 3300));
  assert(!gate.accept("yes", 0.99f, 3400)); // silence alone cannot bypass cooldown
  assert(gate.accept("yes", 0.99f, 4700));
  assert(jr_yes_no::kBeepFrames == 1920);
  assert(jr_yes_no::beep_sample(-1) == 0);
  assert(jr_yes_no::beep_sample(0) == 0);
  assert(jr_yes_no::beep_sample(1919) == 0);
  assert(jr_yes_no::beep_sample(1920) == 0);
  int crossings = 0;
  int32_t previous = 0;
  long long total = 0;
  for (int frame = 0; frame < jr_yes_no::kBeepFrames; ++frame) {
    const int32_t sample = jr_yes_no::beep_sample(frame);
    assert(std::abs(static_cast<long long>(sample)) <= 3932LL * 65536);
    total += std::abs(static_cast<long long>(sample));
    if (previous <= 0 && sample > 0) ++crossings;
    previous = sample;
  }
  assert(total > 1000000000LL);
  assert(crossings >= 105 && crossings <= 107); // 880Hz * .120s
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            cpp, exe = Path(tmp) / "test.cc", Path(tmp) / "test"
            cpp.write_text(source)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            "-fsanitize=undefined", "-I", str(ROOT / "experiments/tflite-yes-no"),
                            str(cpp), "-o", str(exe)], check=True, capture_output=True)
            subprocess.run([str(exe)], check=True, capture_output=True)


if __name__ == "__main__":
    unittest.main()
