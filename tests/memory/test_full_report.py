import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


@unittest.skipUnless(sys.platform.startswith('linux') and shutil.which('cc'), 'Host Linux C compiler required')
class FullReportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.executables = []
        symbols = {'_iram_start': 0x40378000, '_iram_end': 0x40384000,
                   '_data_start': 0x3fc94000, '_heap_start': 0x3fc9c000,
                   '_data_end': 0x3fc95000, '_bss_start': 0x3fc95000,
                   '_bss_end': 0x3fc9bf00, '_noinit_start': 0x3fc9bf00,
                   '_noinit_end': 0x3fc9c000}
        for tracing in (1, 0):
            executable = Path(cls.temp.name) / ('memory_full_' + str(tracing))
            command = ['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-no-pie', '-fsanitize=address,undefined',
                       '-DCONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768', '-DCONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=16',
                       '-DCONFIG_HEAP_TASK_TRACKING=' + str(tracing), '-Itests/memory', '-Ifirmware/core',
                       'tests/memory/memory_full_test.c', 'firmware/core/jr_memory_full.c']
            command += ['-Wl,--defsym,' + name + '=' + hex(value) for name, value in symbols.items()]
            command += ['-o', str(executable)]
            subprocess.run(command, check=True, capture_output=True)
            cls.executables.append(executable)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def report(self, index, nested=False, bad_nested=False):
        env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
        if nested:
            env['JR_TEST_NESTED'] = '1'
        if bad_nested:
            env['JR_TEST_BAD_NESTED'] = '1'
        output = subprocess.check_output([str(self.executables[index])], env=env, text=True)
        return json.loads(output)

    def test_physical_sram_balance_excludes_rtc(self):
        report = self.report(0)
        layout = report['layout']
        total = sum(layout[key] for key in ('icache', 'dcache', 'iram_image', 'dram_image',
                                           'heap_allocated_blocks', 'heap_free_blocks', 'heap_admin', 'unclassified'))
        self.assertEqual(total, 512 * 1024)
        self.assertEqual(layout['rtc_heap_extent'], 8192)
        self.assertEqual(layout['unclassified'], 0)
        self.assertEqual(layout['heap_admin'], 128)

    def test_nested_dma_pool_is_not_counted_twice(self):
        report = self.report(0, nested=True)
        layout = report['layout']
        self.assertTrue(layout['accounting_consistent'])
        self.assertEqual(layout['heap_nested_bytes'], 32768)
        self.assertEqual(layout['heap_registered_extent'] - layout['heap_extent'], 32768)
        self.assertEqual(layout['heap_allocated_blocks'], 32768)
        self.assertEqual(layout['heap_free_blocks'], 278400 + 32024)
        self.assertEqual(layout['heap_admin'], 128 + 744)
        self.assertEqual(layout['unclassified'], 0)
        self.assertEqual(report['regions'][0]['nested_used'], 32768)

    def test_inconsistent_overlap_stays_explicit(self):
        report = self.report(0, nested=True, bad_nested=True)
        self.assertFalse(report['layout']['accounting_consistent'])

    def test_compiled_limits_are_not_invented(self):
        config = self.report(0)['allocation_config']
        self.assertEqual(config['reserve_internal_bytes'], 32768)
        self.assertEqual(config['wifi_static_rx_count'], 16)
        self.assertNotIn('tls_out_limit_bytes', config)

    def test_names_blocks_and_caps(self):
        report = self.report(0)
        self.assertTrue(report['complete'])
        self.assertTrue(report['tracking_enabled'])
        self.assertEqual(report['tasks'][0]['name'], 'httpd"\\\n')
        self.assertEqual(report['tasks'][0]['heaps'][0]['blocks'], [[0x3fc9c010, 65500]])
        self.assertTrue(report['tasks'][1]['heaps'][0]['caps'] & report['caps']['rtc'])

    def test_trace_disabled_is_explicit(self):
        report = self.report(1)
        self.assertFalse(report['tracking_enabled'])
        self.assertFalse(report['complete'])
        self.assertEqual(report['tasks'], [])


if __name__ == '__main__':
    unittest.main()
