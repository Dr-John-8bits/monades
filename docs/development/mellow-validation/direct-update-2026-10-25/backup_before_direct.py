import sys,time,json,hashlib
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from device_editor import Editor
from hardware_bench import Device
from dreams_backup import B,KIND
import fm1_install as I
out=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Direct-Upgrade')
assert not (out/'live.raw').exists(),'Preserve earlier snapshot'
d=Editor();d.status=Device.status.__get__(d)
try:
 status=d.status();live=d.snapshot()
 (out/'live.raw').write_bytes(live)
 (out/'live.json').write_text(json.dumps(dict(format='mutable-dreams-patch',version=1,firmware='1.1-dev',name='After owner live-editor test',patch=list(live)),indent=2))
 (out/'status.json').write_text(json.dumps(status,indent=2))
finally:d.close()
for suffix in ['a','b']:
 with (out/f'backup-{suffix}.log').open('w') as log:B.backup(I.MidoBackend(),out/f'Banks-{suffix}.mddb','X0X FM-1',log,KIND)
assert (out/'Banks-a.mddb').read_bytes()==(out/'Banks-b.mddb').read_bytes()
(out/'SHA256SUMS.txt').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS.txt'))
d=Editor()
try:d.command(27,0,0);d.command(61,0,0)
finally:d.close()
print('PASS fresh post-editor live state saved and two complete bank backups identical. Device stopped; no saved data changed.',flush=True)
