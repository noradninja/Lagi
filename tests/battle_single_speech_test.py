from pathlib import Path
import subprocess,sys,tempfile
r=Path(__file__).resolve().parents[1]
assert len(sys.argv)==2,'Pass original BTL_A5_3.PRG'
b=Path(sys.argv[1]).read_bytes();base=0x06054000
u=lambda ea,n=2:int.from_bytes(b[ea-base:ea-base+n],'big')
for ea,value,n in [(0x06068B16,0x600E,2),(0x06068B18,0x4011,2),
 (0x06068BD0,0x06068B84,4),(0x06068B72,0xE0FF,2),
 (0x06068B78,0xBFBE,2),(0x06068BB8,0xCB01,2),
 (0x060555B6,0xE50A,2),(0x06055634,0x060A3F64,4),
 (0x06055638,0x06068B66,4)]:
 assert u(ea,n)==value,f'Original single speech contract differs at {ea:08X}'
s=(r/'build-vita/azel_upstream/battle_battleFormationIntroEffect.cpp').read_text()
a=s.index('static void lagiFormationIntroEffectUpdateBody(');z=s.index('// part of 06077384',a)
assert 'formationIntroEffect_HandleSkipKey' not in s[a:z]
with tempfile.TemporaryDirectory(prefix='lagi-single-speech-') as tmp:
 d=Path(tmp);(d/'speech_body.inc').write_text(s[a:z]);(d/'single_speech.inc').write_text((r/'src/integration/lagi_battle_single_speech.inc').read_text())
 subprocess.run(['cl','/nologo','/std:c++20','/EHsc','/I'+str(d),str(r/'tests/battle_single_speech_test.cpp'),'/Fe:'+str(d/'test.exe'),'/Fo:'+str(d/'test.obj')],check=True)
 subprocess.run([str(d/'test.exe')],check=True)
print('PASS original negative-flag task selection and Lathum speech factory contracts')
