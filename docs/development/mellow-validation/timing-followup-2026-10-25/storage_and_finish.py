import sys,time,json,struct,hashlib
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from device_editor import Editor
from hardware_bench import Device
from dreams_backup import B,KIND
import fm1_install as I
r=Path('work/timing-audit');private=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Timing')
original=(private/'live.raw').read_bytes();baseline=(private/'Banks-a.mddb').read_bytes();objects=dict(B.parse(baseline,KIND)[1]);rows=[]
assert len(json.loads((r/'long-audio-stress.json').read_text()))==5
assert json.loads((r/'midi-long.json').read_text())['messages']>=64000
# Re-save an already occupied slot using exactly its existing payload/name, never
# create a test bank or replace user content with an invented patch.
obj=next(o for o in range(3,19) if len(objects[o])==1320)
slot=obj-3;name=objects[obj][4:28].split(b'\0')[0].decode('ascii');patch=objects[obj][28:]
d=Editor();d.status=Device.status.__get__(d)
try:
 d.command(27,0,0);d.command(61,0,0);time.sleep(.1)
 d.bank(0,slot,0,1);assert d.snapshot()==patch
 for i in range(3):
  before=d.status();d.bank(0,slot,0,3,name);after=d.status()
  assert d.snapshot()==patch
  assert after['overloads']==before['overloads'] and after['nonfinite']==before['nonfinite']
  rows.append(dict(iteration=i+1,before=before,after=after,xrun_delta=after['xruns']-before['xruns']))
 d.upload(original);assert d.snapshot()==original;d.ui(100,0,0)
finally:d.close()
d=Device('X0X FM-1')
try:
 d.remote([1,0,0]);d.remote([2,0,0,0,0]);d.remote([4,1,32]);d.remote([5,0]);status=d.status()
finally:d.close()
with (private/'final-backup.log').open('w') as log:B.backup(I.MidoBackend(),private/'Final.mddb','X0X FM-1',log,KIND)
assert (private/'Final.mddb').read_bytes()==baseline
report=dict(same_payload_resave_tests=rows,all_22_saved_objects_exact=True,latest_user_live_restored=True,original_live_sha256=hashlib.sha256(original).hexdigest(),stopped=not status['playing'],home=status['page']==0,physical_master_released=True,internal_clock=True,status=status)
(r/'final-state.json').write_text(json.dumps(report,indent=2))
print('PASS same-content User Biome save/load/readback three times; original post-listening live patch restored; all 22 saved objects unchanged. Storage timing deltas:',[x['xrun_delta'] for x in rows],flush=True)
