import sys,time,json,hashlib,struct,subprocess
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-release-1.1/tools')
from hardware_bench import Device
from device_editor import Editor
from dreams_backup import B,KIND
import fm1_install as I
r=Path('work/release-qualification/release-direct');private=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Release-Label')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
old=Path('outputs/Mutable-Dreams-1.0.fwsc');new=Path('work/mutable-dreams-release-1.1/app/build/mutable-dreams-1.1.0.fwsc')
assert sha(old)=='75008881683cbeb3515e24cfd7830136b90eb293f0752dc66b203b7f2ee7cd9f'
assert sha(new)=='8e039bb0e9681fbbb0504e9b7d251895958fd88ae97f11ff0342b4aef38143d2'
assert len((private/'live.raw').read_bytes())==1292
assert (private/'Banks-a.mddb').read_bytes()==(private/'Banks-b.mddb').read_bytes()
assert len(B.parse((private/'Banks-a.mddb').read_bytes(),KIND)[1])==22
def install(file,label,expected):
 with (r/(label+'.log')).open('w') as log:
  subprocess.run([sys.executable,'work/mutable-dreams-release-1.1/base/tools/fm1_install.py','--yes',str(file)],stdout=log,stderr=subprocess.STDOUT,check=True)
 assert 'done: the FM-1 runs '+expected in (r/(label+'.log')).read_text()
 print('PASS',label,expected,flush=True)
install(old,'install-original-1.0','FM-1_710010000')
d=Device('X0X FM-1')
try:
 d.status();d.command(27,0,0);d.command(20,0,0)
 d.command_many([(15,0,83),(14,0,371),(14,1,432),(14,2,286),(14,3,413),(10,0,47),(11,1,613),(41,2,366)])
 legacy=d.patch();assert len(legacy)==652 and struct.unpack_from('<I',legacy,4)[0]==4
 d.tap(6);d.tap(7);time.sleep(.2)
 size,magic,version=struct.unpack('<III',d.call(6));assert size==652 and version==4
 saved=b''.join(d.call(5,[off&127,off>>7]) for off in range(0,size,224))
 assert saved==legacy
 (r/'legacy-test.raw').write_bytes(legacy)
 (r/'legacy-state.json').write_text(json.dumps(d.status(),indent=2))
 print('PASS original 1.0 custom Biome saved and read back exactly',flush=True)
finally:d.close()
install(new,'direct-1.0-to-1.1','FM-1_710010100')
assert 'device:  FM-1_710010000' in (r/'direct-1.0-to-1.1.log').read_text()
d=Editor();d.status=Device.status.__get__(d)
try:
 live=d.snapshot();status=d.status()
 assert len(live)==1292 and struct.unpack_from('<I',live,4)[0]==6
 assert live[:4]==legacy[:4] and live[8:652]==legacy[8:652]
 assert status['tempo']==83 and not status['playing']
 (r/'migrated-test.raw').write_bytes(live)
 print('PASS all original saved Biome fields migrated exactly from v4 to v6',flush=True)
finally:d.close()
with (private/'after-direct-upgrade.log').open('w') as log:
 B.backup(I.MidoBackend(),private/'After-direct-upgrade.mddb','X0X FM-1',log,KIND)
before=dict(B.parse((private/'Banks-a.mddb').read_bytes(),KIND)[1]);after=dict(B.parse((private/'After-direct-upgrade.mddb').read_bytes(),KIND)[1])
assert after[0]==legacy
preserved=[o for o in before if o!=0 and before[o]==after[o]]
report=dict(route='published Mutable Dreams 1.0 -> exact current 1.1 candidate, without Stock V15',original_firmware_sha256=sha(old),candidate_firmware_sha256=sha(new),legacy_patch_version=4,new_patch_version=6,all_original_fields_exact=True,saved_legacy_retained=True,objects_unchanged_except_test_project=preserved,all_other_objects_preserved=len(preserved)==21,status=status,tool='Python USB-MIDI installer; not a visible browser-installer test',restoration_pending=True)
(r/'direct-upgrade.json').write_text(json.dumps(report,indent=2))
print('PASS direct upgrade; unchanged saved objects excluding deliberately saved test project:',len(preserved),'of 21. Original user restore is still required.',flush=True)
