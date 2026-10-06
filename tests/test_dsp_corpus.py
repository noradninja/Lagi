"""Validation of the LDSP interchange format and content-based generation."""
from pathlib import Path
import struct, sys, tempfile, unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from generate_scspdsp_aot import load_corpus, program_hash, generate

class CorpusTests(unittest.TestCase):
    def test_empty_and_duplicate_corpus(self):
        words=(128,0,0,0)
        with tempfile.TemporaryDirectory() as directory:
            folder=Path(directory)
            self.assertEqual(load_corpus(folder),[])
            blob=struct.pack('<4I4H',0x5053444c,1,1,program_hash(words),*words)
            (folder/'a.ldsp').write_bytes(blob); (folder/'b.ldsp').write_bytes(blob)
            self.assertEqual(load_corpus(folder),[words])

    def test_invalid_captures(self):
        words=(128,0,0,0)
        good=struct.pack('<4I4H',0x5053444c,1,1,program_hash(words),*words)
        bad=[b'',good[:15],good[:-1],good+b'\0',struct.pack('<4I',0,1,0,0),
             struct.pack('<4I',0x5053444c,2,0,0),struct.pack('<4I',0x5053444c,1,129,0),
             struct.pack('<4I4H',0x5053444c,1,1,0,*words)]
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/'bad.ldsp'
            for blob in bad:
                p.write_bytes(blob)
                with self.assertRaises(ValueError): load_corpus(p.parent)

    def test_empty_program_and_live_reads(self):
        schema=Path(__file__).resolve().parents[1]/'src/integration/lagi_dsp_fields.def'
        text=generate([(),(128,1<<13,0,0),(0,0,0,0,128,0,0x6000,0),(0,0,0,0,128,0,0x6000,0)],schema)
        self.assertIn('const volatile INT16',text)
        self.assertIn('const volatile UINT16',text)
        self.assertIn('g_lagiDspAotCount=4',text)
        self.assertIn('__start_lagi_dsp_aot_0_code',text)
        with self.assertRaises(ValueError): generate([(128,63<<6,0,0)],schema)

if __name__=='__main__': unittest.main()
