from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work/mutable-dreams-release-1.1/tools')
from device_editor import Editor
from hardware_bench import Device
from dreams_backup import B,KIND
import fm1_install as I
q=Path('work/release-qualification/release-audio');p=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Release-Label')
assert json.loads((q/'capture-restored.json').read_text())['clips_completed']==7
d=Editor();d.status=Device.status.__get__(d)
try:
    assert d.snapshot()==(p/'live.raw').read_bytes();s=d.status()
    assert not s['playing'] and s['page']==0 and s['overloads']==0 and s['nonfinite']==0 and s['dropped']==0
finally:d.close()
with (p/'after-audio.log').open('w') as log:B.backup(I.MidoBackend(),p/'After-audio.mddb','X0X FM-1',log,KIND)
assert dict(B.parse((p/'After-audio.mddb').read_bytes(),KIND)[1])==dict(B.parse((p/'Banks-a.mddb').read_bytes(),KIND)[1])
(q/'final-device-state.json').write_text(json.dumps(dict(all_22_saved_objects_exact=True,original_live_exact=True,stopped_home=True,device_free=True,status=s),indent=2))
(p/'SHA256SUMS.txt').write_text(''.join(f'{hashlib.sha256(x.read_bytes()).hexdigest()}  {x.name}\n' for x in sorted(p.iterdir()) if x.is_file() and x.name!='SHA256SUMS.txt'))
print('PASS after final recordings: all 22 saved objects and original unsaved live exact; HOME/stopped, device free.')
