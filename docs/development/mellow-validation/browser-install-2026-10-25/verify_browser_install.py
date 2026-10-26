"""Run only after the owner confirms installer completion and device is free."""
import sys,json,hashlib
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from hardware_bench import Device
from device_editor import Editor
from dreams_backup import B,KIND
import fm1_install as I
r=Path('work/release-qualification');p=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Direct-Upgrade')
expected=dict(B.parse((p/'Banks-a.mddb').read_bytes(),KIND)[1])
d=Editor();d.status=Device.status.__get__(d)
try:
 status=d.status();boot=d.snapshot();assert boot==expected[0],'Startup project differs; preserve and investigate before restoring anything'
 assert not status['playing']
finally:d.close()
with (p/'after-browser-install.log').open('w') as log:B.backup(I.MidoBackend(),p/'After-browser-install.mddb','X0X FM-1',log,KIND)
actual=dict(B.parse((p/'After-browser-install.mddb').read_bytes(),KIND)[1]);assert actual==expected
d=Editor()
try:d.upload((p/'live.raw').read_bytes());assert d.snapshot()==(p/'live.raw').read_bytes();d.ui(100,0,0)
finally:d.close()
d=Device('X0X FM-1')
try:d.remote([1,0,0]);d.remote([2,0,0,0,0]);d.remote([4,1,32]);d.remote([5,0])
finally:d.close()
(r/'browser-install-readback.json').write_text(json.dumps(dict(all_22_saved_objects_exact=True,startup_project_exact=True,post_editor_live_restored=True,status=status),indent=2))
(p/'SHA256SUMS.txt').write_text(''.join(f'{hashlib.sha256(x.read_bytes()).hexdigest()}  {x.name}\n' for x in sorted(p.iterdir()) if x.is_file() and x.name!='SHA256SUMS.txt'))
print('PASS browser reinstall readback: startup project/all 22 saved objects exact; latest user live patch restored.',flush=True)
