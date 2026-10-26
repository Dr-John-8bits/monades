import sys,time,json,hashlib
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from device_editor import Editor
from hardware_bench import Device
from dreams_backup import B,KIND
import fm1_install as I
r=Path('work/release-qualification');p=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Direct-Upgrade')
j=json.loads((r/'direct-upgrade.json').read_text());assert j['all_original_fields_exact'] and j['all_other_objects_preserved']
original=dict(B.parse((p/'Banks-a.mddb').read_bytes(),KIND)[1]);assert len(original[0])==1292
# All User Banks/preferences already survived. Restore only the one deliberately
# changed project via normal live upload + SAVE, not a full-bank restore.
d=Editor()
try:
 d.command(27,0,0);d.command(61,0,0);d.upload(original[0]);assert d.snapshot()==original[0];d.ui(100,0,0)
finally:d.close()
d=Device('X0X FM-1')
try:d.status();d.tap(7)
finally:d.close()
with (p/'after-project-restore.log').open('w') as log:B.backup(I.MidoBackend(),p/'After-project-restore.mddb','X0X FM-1',log,KIND)
restored=dict(B.parse((p/'After-project-restore.mddb').read_bytes(),KIND)[1]);assert restored==original
d=Editor();d.status=Device.status.__get__(d)
try:
 live=(p/'live.raw').read_bytes();d.upload(live);assert d.snapshot()==live;d.ui(100,0,0);status=d.status()
finally:d.close()
d=Device('X0X FM-1')
try:d.remote([1,0,0]);d.remote([2,0,0,0,0]);d.remote([4,1,32]);d.remote([5,0])
finally:d.close()
j.update(restoration_pending=False,all_22_original_objects_restored=True,post_editor_live_restored=True,restoration='Single saved project restored through normal live import + SAVE. Other 21 objects were already unchanged; no full-bank restore needed.',restored_status=status)
(r/'direct-upgrade.json').write_text(json.dumps(j,indent=2))
(p/'SHA256SUMS.txt').write_text(''.join(f'{hashlib.sha256(x.read_bytes()).hexdigest()}  {x.name}\n' for x in sorted(p.iterdir()) if x.is_file() and x.name!='SHA256SUMS.txt'))
print('PASS all 22 saved objects exactly restored; latest post-editor live patch restored; HOME, stopped, internal clock and physical controls released.',flush=True)
