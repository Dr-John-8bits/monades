import sys,time,json,io
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from device_editor import Editor
from hardware_bench import Device
from dreams_backup import B,KIND
import fm1_install as I
out=Path('work/timing-audit');private=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Timing')
d=Device('X0X FM-1')
try:before=d.status();assert not before['playing']
finally:d.close()
print('BEFORE',before['xruns'],flush=True)
result=B.restore(I.MidoBackend(),private/'Banks-a.mddb','X0X FM-1',True,out=sys.stdout,kind=KIND)
assert result==1,'Expected physical confirmation timeout (no user action)'
d=Device('X0X FM-1')
try:after=d.status()
finally:d.close()
with (private/'timeout-backup.log').open('w') as log:B.backup(I.MidoBackend(),private/'After-timeout.mddb','X0X FM-1',log,KIND)
assert (private/'After-timeout.mddb').read_bytes()==(private/'Banks-a.mddb').read_bytes()
report=dict(before=before,after=after,xrun_delta=after['xruns']-before['xruns'],cancelled_by_timeout=True,all_saved_objects_unchanged=True)
(out/'restore-timeout.json').write_text(json.dumps(report,indent=2));print('RESULT',report['xrun_delta'],'timing events during stopped staging/cancellation; all saved objects unchanged',flush=True)
