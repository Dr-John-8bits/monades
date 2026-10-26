import sys,json,hashlib,struct,time
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from device_editor import Editor
from hardware_bench import Device
from dreams_backup import B,KIND
import fm1_install as I
out=Path('work/timing-audit');private=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Timing')
assert not (private/'live.raw').exists(),'Do not overwrite the pre-test snapshot'
d=Editor();d.status=Device.status.__get__(d);rows=[]
def mark(name):
 row=dict(name=name,status=d.status());rows.append(row);print(name,row['status']['xruns'],flush=True);return row
try:
 mark('before-any-test');live=d.snapshot();state=list(struct.unpack('<46i',d.call(9)[:184]));
 (private/'live.raw').write_bytes(live);(private/'live.json').write_text(json.dumps(dict(format='mutable-dreams-patch',version=1,firmware='1.1-dev',name='Before timing audit',patch=list(live)),indent=2))
 (private/'state.json').write_text(json.dumps(state));mark('after-read-only-live-snapshot')
finally:d.close()
for suffix in ['a','b']:
 with (private/f'backup-{suffix}.log').open('w') as log:B.backup(I.MidoBackend(),private/f'Banks-{suffix}.mddb','X0X FM-1',log,KIND)
assert (private/'Banks-a.mddb').read_bytes()==(private/'Banks-b.mddb').read_bytes()
d=Editor();d.status=Device.status.__get__(d)
try:
 mark('after-two-read-only-full-backups');d.command(27,0,0);d.command(61,0,0);time.sleep(.1);mark('stopped-internal')
 for i in range(6):
  d.upload(live);assert d.snapshot()==live;mark('live-upload-'+str(i+1))
 for page in range(19):d.ui(100,0,page);d.call(9)
 mark('after-all-pages')
 for i in range(3):
  d.command(20,0,73);time.sleep(.1);mark('factory-biome-'+str(i+1))
 d.upload(live);d.ui(100,0,0);mark('original-live-restored')
finally:d.close();(out/'initial.json').write_text(json.dumps(rows,indent=2))
(private/'SHA256SUMS.txt').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in sorted(private.iterdir()) if p.is_file() and p.name!='SHA256SUMS.txt'))
print('PASS fresh backup twice identical, initial phase observations saved.',flush=True)
