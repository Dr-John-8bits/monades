import sys,time,json
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from hardware_bench import Device
d=Device('X0X FM-1')
try:
 d.command(27,0,0);d.command(61,0,0);d.command(20,0,73);d.tap(6)
 d.remote([1,0,0]);d.remote([2,0,0,0,0])
 # 4097 releases the bench master override; the physical pot owns volume again.
 d.remote([4,1,32]);time.sleep(.25);d.call(3)
 s=d.status();assert not s['playing'] and s['page']==0 and s['help']==-1
 Path('work/device-mellow/final-device-status.json').write_text(json.dumps(dict(status=s,master_override_released=True,live_biome='Satin River',saved_startup_project_overwritten=False),indent=2))
 print('READY: Satin River stopped, HOME, physical master restored',s['master'])
finally:d.close()
