import sys,time,json,struct,hashlib
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from hardware_bench import Device,Meter
import mido
r=Path('work/device-mellow');d=Device('X0X FM-1');meter=Meter();checks=[]
def check(name,ok):
 assert ok,name
 checks.append(name);print('PASS',name,flush=True)
def snap(obj):
 n=struct.unpack_from('<I',d.call(6,[obj]))[0]
 return b''.join(d.call(5,[off&127,off>>7]) for off in range(0,n,224))
try:
 d.status();d.command(27,0,0);d.command(61,0,0)
 backup=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-24/Before-Hub-Audit')
 for obj,name in [(0,'FM1-Gespeichertes-Projekt.raw'),(18,'User-Biome-16.raw'),(22,'User-Tones-04.raw')]:
  check('saved object unchanged across audit updates: '+name,snap(obj)==(backup/name).read_bytes())
 d.tap(6)
 for _ in range(5):d.tap(5)
 for role,delta in [(4,-1),(3,-63),(3,15),(5,1)]:d.remote([3,role,delta+64]);time.sleep(.12)
 check('own named Biome and sequence recall after update',d.patch()==(backup/'User-Biome-16.raw').read_bytes()[28:])
 d.tap(6)
 d.remote([4,0,16]);d.command(20,0,64)
 for ch in range(6):d.command_many([(45,ch,208+ch),(12,ch,0),(4,ch,400),(43,ch,1),(5,ch,0)])
 d.command_many([(51,0,0),(14,3,0)]);d.call(3);base=d.status();meter.stream.start()
 start=time.monotonic();loops=0
 # 18 messages per cycle, paced at 30 Hz (~540 messages/s), over six channels.
 # This is a sustained traffic test, not an assertion of maximum USB capacity.
 while time.monotonic()-start<20:
  for ch in range(6):
   note=48+ch
   d.o.send(mido.Message('note_on',channel=ch,note=note,velocity=70+(loops%40)))
   d.o.send(mido.Message('control_change',channel=ch,control=74,value=40+loops%60))
   d.o.send(mido.Message('note_off',channel=ch,note=note,velocity=0))
  loops+=1;time.sleep(max(0,start+loops/30-time.monotonic()))
 time.sleep(.25);end=d.status();audio=meter.snapshot()
 check('all six channels received every scheduled Note On',all(y-x==loops for x,y in zip(base['events'],end['events'])))
 for field in ['xruns','overloads','nonfinite','dropped']:check('no new '+field,end[field]==base[field])
 check('USB capture finite and uninterrupted',audio['nonfinite']==0 and audio['host_input_overflows']==0 and audio['frames']>19*44100)
 check('six-channel MIDI audio audible',audio['energy']>1e-4)
 # Snapshot protection while externally clocked, even when transport stopped.
 saved=snap(0);d.command(61,0,1);d.tap(6);d.tap(7);d.command(61,0,0)
 check('main SAVE refused while external clock selected',snap(0)==saved)
 d.command(27,0,0);d.tap(6)
 for b in range(14):
  before=d.patch();d.remote([1,(1<<b)&127,(1<<b)>>7]);time.sleep(.95)
  check('held help '+str(b),d.status()['help']==b)
  d.remote([1,0,0]);time.sleep(.12)
  check('release only, no short action '+str(b),d.patch()==before);d.tap(6)
 report=dict(firmware_sha256=hashlib.sha256(Path('work/device-mellow/plectrum-before-organic-glass.fwsc').read_bytes()).hexdigest(),checks=checks,cycles=loops,messages=loops*18,baseline=base,final=end,audio=audio)
 (r/'midi-plectrum-final.json').write_text(json.dumps(report,indent=2))
finally:
 d.remote([1,0,0]);d.command(27,0,0);d.command(61,0,0);d.remote([4,1,32]);d.close();meter.stream.stop();meter.stream.close()
