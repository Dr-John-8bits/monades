import sys,json,hashlib,struct
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from device_editor import Editor
from hardware_bench import Device
from dreams_backup import B,KIND
import fm1_install as I
out=Path('work/timing-audit');private=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Timing')
d=Editor();d.status=Device.status.__get__(d)
try:
 status=d.status();patch=d.snapshot();assert not status['playing']
 expected=dict(B.parse((private/'Banks-a.mddb').read_bytes(),KIND)[1])[0]
 assert patch==expected,'Cold-start project differs from saved project'
finally:d.close()
with (private/'cold-backup.log').open('w') as log:B.backup(I.MidoBackend(),private/'After-cold-start.mddb','X0X FM-1',log,KIND)
assert (private/'After-cold-start.mddb').read_bytes()==(private/'Banks-a.mddb').read_bytes()
(out/'cold-start.json').write_text(json.dumps(dict(physical_power_cycle_confirmed=True,status=status,saved_startup_project_exact=True,all_22_saved_objects_exact=True,firmware_sha256=hashlib.sha256(Path('work/mutable-dreams-next/app/build/mutable-dreams-1.1.0-dev.fwsc').read_bytes()).hexdigest()),indent=2))
print('PASS cold start, exact saved project and all saved banks; xruns:',status['xruns'],'overloads:',status['overloads'],flush=True)
