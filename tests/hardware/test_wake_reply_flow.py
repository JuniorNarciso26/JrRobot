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


    def test_fast_wake_cache_keeps_original_feedback(self):
        self.assertIn("jr_tts_prepare_wake_reply();", VOICE)
        self.assertIn("xTaskCreatePinnedToCore(wake_cache_task", TTS)
        self.assertIn("JR_TTS_WAKE_CACHE_READY", TTS)
        self.assertIn("JR_TTS_WAKE_CACHE_HIT", TTS)
        self.assertIn("JR_TTS_WAKE_CACHE_PREPARE", TTS)
        self.assertIn('JR_WAKE_REPLY_TEXT "Olá! Eu sou o JrBot."', TTS)
        self.assertIn("cache_reply_pcm(c.pcm, c.count, model->sample_rate);", TTS)
        self.assertIn("jr_resource_i2s_acquire(JR_I2S_OWNER_TTS, 0, \"tts_wake_cached\")", TTS)
        self.assertIn("jr_resource_i2s_release(JR_I2S_OWNER_TTS, \"tts_wake_cached_done\")", TTS)

    def test_cache_prewarm_never_uses_mic_audio_bus(self):
        start = TTS.index("static esp_err_t synth_and_play(")
        end = TTS.index("static void wake_cache_task(", start)
        body = TTS[start:end]
        self.assertIn("if (!cache_only) {", body)
        self.assertIn("jr_resource_i2s_acquire(JR_I2S_OWNER_TTS, 0, \"tts_speak\")", body)
        self.assertIn("if (cache_only || job->wake_reply)", body)
        self.assertIn("wake_cache.finished = true", TTS)

    def test_tts_yields_and_usb_events_do_not_block_audio(self):
        self.assertIn("JR_TTS_COOPERATIVE_YIELD_US", TTS)
        self.assertIn("vTaskDelay(1);", TTS)
        usb = (ROOT / "firmware/core/jr_usb_terminal.c").read_text(encoding="utf-8")
        event = usb.split("void jr_usb_terminal_emit(const char *text) {", 1)[1]
        event = event.split("static bool brain_command(", 1)[0]
        self.assertIn("usb_serial_jtag_write_bytes(wire, len, pdMS_TO_TICKS(20))", event)
        self.assertNotIn("usb_serial_jtag_wait_tx_done", event)


if __name__ == "__main__":
    unittest.main()
