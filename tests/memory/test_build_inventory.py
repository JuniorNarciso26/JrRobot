import importlib.util
import struct
import unittest
import tempfile
import json
from unittest.mock import patch
from pathlib import Path

spec = importlib.util.spec_from_file_location('build_memory_report', Path('tools/build_memory_report.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def fixture():
    names = ['.shstrtab', '.strtab', '.symtab', '.iram0.text', '.dram0.data', '.dram0.bss',
             '.dram0.dummy', '.flash.text', '.ext_ram.bss', '.rtc.bss']
    shstrings = b'\0'
    offsets = {}
    for name in names:
        offsets[name] = len(shstrings)
        shstrings += name.encode() + b'\0'
    strings = b'\0'
    symbols = b'\0' * 16

    def symbol(name, address, size, section, kind=1):
        nonlocal strings, symbols
        at = len(strings)
        strings += name.encode() + b'\0'
        symbols += struct.pack('<IIIBBH', at, address, size, kind, 0, section)

    symbol('_iram_start', 0x40378000, 0, 0xfff1)
    symbol('_iram_end', 0x40378100, 0, 0xfff1)
    symbol('_data_start', 0x3fc88100, 0, 0xfff1)
    symbol('_heap_start', 0x3fc88160, 0, 0xfff1)
    symbol('ram_function', 0x40378000, 256, 4, 2)
    symbol('settings', 0x3fc88100, 32, 5)
    symbol('big_buffer', 0x3fc88120, 64, 6)
    symbol('alias_buffer', 0x3fc88120, 64, 6)
    symbol('flash_function', 0x42000020, 64, 8, 2)
    symbol('external_buffer', 0x3c100000, 16, 9)
    symbol('rtc_buffer', 0x600fe000, 16, 10)
    blob = bytearray(52)
    sections = [(0,) * 10]

    def section(name, kind, flags, address, content, size=None, link=0, entsize=0):
        offset = len(blob)
        blob.extend(content)
        sections.append((offsets[name], kind, flags, address, offset,
                         len(content) if size is None else size, link, 0, 4, entsize))

    section('.shstrtab', 3, 0, 0, shstrings)
    section('.strtab', 3, 0, 0, strings)
    section('.symtab', 2, 0, 0, symbols, link=2, entsize=16)
    section('.iram0.text', 1, 6, 0x40378000, b'\0' * 256)
    section('.dram0.data', 1, 3, 0x3fc88100, b'\0' * 32)
    section('.dram0.bss', 8, 3, 0x3fc88120, b'', size=64)
    section('.dram0.dummy', 8, 3, 0x3fc88000, b'', size=256)
    section('.flash.text', 1, 6, 0x42000020, b'\0' * 64)
    section('.ext_ram.bss', 8, 3, 0x3c100000, b'', size=16)
    section('.rtc.bss', 8, 3, 0x600fe000, b'', size=16)
    shoff = len(blob)
    for section in sections:
        blob.extend(struct.pack('<IIIIIIIIII', *section))
    ident = b'\x7fELF\x01\x01\x01' + b'\0' * 9
    blob[:52] = struct.pack('<16sHHIIIIIHHHHHH', ident, 2, 94, 1, 0, 0, shoff, 0, 52, 0, 0, 40, len(sections), 1)
    return bytes(blob)


MAP = '''Discarded input sections
 .bss.discarded 0x3fc88120 0x40 bad/libwrong.a(wrong.c.obj)
Linker script and memory map
.iram0.text 0x40378000 0x100
 .iram0.text.ram_function
                0x40378000 0x100 esp-idf/heap/libheap.a(heap_caps.c.obj)
.dram0.data 0x3fc88100 0x20
 .data.settings 0x3fc88100 0x20 esp-idf/main/libmain.a(main.c.obj)
.dram0.bss 0x3fc88120 0x40
 .bss.big_buffer 0x3fc88120 0x40 esp-idf/wifi/libwifi.a(wifi.c.obj)
'''


class InventoryTests(unittest.TestCase):
    def test_sections_symbols_and_components(self):
        report = module.inventory(fixture(), MAP, 'test')
        self.assertEqual(report['static_section_bytes'], 352)
        self.assertEqual(report['map_attributed_bytes'], 352)
        self.assertEqual(report['iram_reserved'], 256)
        self.assertEqual(report['dram_reserved'], 96)
        self.assertEqual(report['components'][0], {'name': 'libheap.a', 'size': 256})
        self.assertNotIn('flash_function', [s['name'] for s in report['symbols']])
        self.assertNotIn('.dram0.dummy', [s['name'] for s in report['sections']])
        self.assertEqual(next(s for s in report['symbols'] if s['name'] == 'big_buffer')['owner'],
                         'libwifi.a(wifi.c.obj)')
        self.assertEqual(report['map_overlap_bytes'], 0)
        self.assertEqual([s for s in report['symbols'] if s['name'] == 'rtc_buffer'][0]['kind'], 'rtc')
        self.assertEqual(module.inventory(fixture(), MAP, 'test'), report)

    def test_missing_map_keeps_unknown_explicit(self):
        report = module.inventory(fixture(), '', 'test')
        self.assertEqual(report['map_attributed_bytes'], 0)
        self.assertEqual(report['components'], [])
        self.assertEqual(report['symbols'][0]['owner'], 'Nao identificado no map')

    def test_truncated_elf_rejected(self):
        with self.assertRaises((ValueError, struct.error)):
            module.parse_elf(fixture()[:60])

    def test_unsupported_elf_rejected(self):
        with self.assertRaises(ValueError):
            module.parse_elf(b'not an elf')

    def test_map_overlap_not_double_counted(self):
        report = module.inventory(fixture(), MAP + ' .bss.alias_buffer 0x3fc88120 0x40 esp-idf/main/libmain.a(alias.c.obj)\n', 'test')
        self.assertEqual(report['map_attributed_bytes'], 352)
        self.assertEqual(report['map_overlap_bytes'], 64)

    def test_installer_report_relinked_and_verified(self):
        with tempfile.TemporaryDirectory() as temp:
            build = Path(temp)
            (build / 'project_description.json').write_text('{"project_elf":"robot.elf"}')
            (build / 'robot.elf').write_bytes(fixture())
            (build / 'robot.map').write_text(MAP)
            with patch('sys.argv', ['build_memory_report.py', '--build-dir', temp, '--sdkconfig', 'sdkconfig.test', '--rebuild']), \
                 patch.dict('os.environ', {'IDF_PATH': 'directory with spaces'}), patch.object(module.subprocess, 'run') as run:
                self.assertEqual(module.main(), 0)
                run.assert_called_once()
                self.assertIn('SDKCONFIG=sdkconfig.test', run.call_args.args[0])
                self.assertTrue(json.loads((build / 'jr_memory_build.json').read_text())['ready'])

    def test_nonconverging_inventory_rejected(self):
        report = module.inventory(fixture(), MAP, 'test')
        with tempfile.TemporaryDirectory() as temp, \
             patch('sys.argv', ['build_memory_report.py', '--build-dir', temp, '--sdkconfig', 'sdkconfig.test', '--rebuild']), \
             patch.dict('os.environ', {'IDF_PATH': 'test'}), patch.object(module.subprocess, 'run'), \
             patch.object(module, 'load_report', side_effect=[dict(report, layout_hash=str(n)) for n in range(4)]):
            with self.assertRaises(RuntimeError):
                module.main()


if __name__ == '__main__':
    unittest.main()
