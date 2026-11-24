"""Static regression guard for the experimental WakeNet response ordering.

Run with: python -m unittest discover -s tests/hardware -p test_wake_reply_flow.py
A real ESP32-S3 HW04 test is still required for audio timing and I2S.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
VOICE = (ROOT / "firmware/module_brain/jr_voice.c").read_text(encoding="utf-8")
TTS = (ROOT / "firmware/module_tts/jr_tts.c").read_text(encoding="utf-8")


class WakeReplyFlowTest(unittest.TestCase):
    def test_beep_and_love_are_not_replaced_by_speech(self):
        start = VOICE.index("static esp_err_t reply_and_resume(void)")
        end = VOICE.index("static bool handle_wakenet_frame(void)", start)
        flow = VOICE[start:end]
        sequence = [
            "close_mic();",
            'jr_audio_test_tone(880, 180);',
            'wake_callback("Ola ESP", wn_model_name, 1);',
            'jr_tts_reply_to_wake("Olá! Eu sou o JrBot.");',
            "open_mic(sample_rate);",
            "clean_detector();",
            "WAKE_LISTENING_RESUMED",
        ]
        indices = [flow.index(item) for item in sequence]
        self.assertEqual(indices, sorted(indices))
        self.assertIn("WAKE_REPLY_VTTS_FAILED beep_and_face_preserved=1", flow)
        self.assertIn('if (beep_err == ESP_OK && !stop_requested && wake_callback)', flow)

    def test_wake_tts_hands_i2s_back_before_mic_reopens(self):
        self.assertIn("jr_resource_i2s_release(JR_I2S_OWNER_TTS", TTS)
        self.assertIn("xTaskNotifyGive(wake_waiter)", TTS)
        self.assertIn("ulTaskNotifyTake(pdTRUE, portMAX_DELAY)", TTS)
        self.assertIn("if (jr_brain_enabled() && !from_wake)", TTS)
        self.assertIn("if ((!state.settings.enabled && !from_wake) || state.busy)", TTS)

    def test_original_oled_love_duration_is_preserved(self):
        brain = (ROOT / "firmware/module_brain/jr_brain.c").read_text(encoding="utf-8")
        self.assertIn('jr_face_set_temporary_expression("love", 3000U)', brain)


if __name__ == "__main__":
    unittest.main()
