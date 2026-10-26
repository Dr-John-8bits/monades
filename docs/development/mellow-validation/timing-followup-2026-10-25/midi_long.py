import sys,time,json,hashlib
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from hardware_bench import Device,Meter
import mido
r=Path('work/timing-audit');d=Device('X0X FM-1');meter=Meter();checks=[]
def check(name,ok):
 assert ok,name
 checks.append(name);print('PASS',name,flush=True)
try:
 d.status();d.command(27,0,0);d.command(61,0,0);d.remote([4,0,16]);d.command(20,0,64)
 for ch in range(6):d.command_many([(45,ch,208+ch),(12,ch,0),(4,ch,400),(43,ch,1),(5,ch,0)])
 d.command_many([(51,0,0),(14,3,0)]);d.call(3);base=d.status();meter.stream.start()
 start=time.monotonic();loops=0;progress=0
 while time.monotonic()-start<120:
  for ch in range(6):
   note=48+ch
   d.o.send(mido.Message('note_on',channel=ch,note=note,velocity=70+loops%40))
   d.o.send(mido.Message('control_change',channel=ch,control=74,value=40+loops%60))
   d.o.send(mido.Message('note_off',channel=ch,note=note,velocity=0))
  loops+=1;time.sleep(max(0,start+loops/30-time.monotonic()))
  if time.monotonic()-start-progress>=30:progress=time.monotonic()-start;print('PROGRESS MIDI',round(progress),'seconds',loops*18,'messages',flush=True)
 time.sleep(.25);end=d.status();audio=meter.snapshot()
 check('all six channels received every scheduled Note On',all(y-x==loops for x,y in zip(base['events'],end['events'])))
 for field in ['xruns','overloads','nonfinite','dropped','quality_reductions']:check('no new '+field,end[field]==base[field])
 check('USB capture finite and uninterrupted',audio['nonfinite']==0 and audio['host_input_overflows']==0 and audio['frames']>119*44100)
 check('six-channel MIDI audio audible',audio['energy']>1e-4)
 check('MIDI audio retains callback reserve',end['callback_peak_us']/end['callback_budget_us']<.85)
 # Actual USB realtime messages, including transport stop and clock-loss safety.
 d.command(20,0,73);d.command(61,0,1);d.o.send(mido.Message('start'))
 t=time.monotonic()
 for i in range(96):
  d.o.send(mido.Message('clock'));time.sleep(max(0,t+(i+1)/48-time.monotonic()))
 clock=d.status();check('USB MIDI Start and 24-PPQN clock play',clock['playing']==1 and abs(clock['tempo']-120)<=3)
 d.o.send(mido.Message('stop'));time.sleep(.08);check('USB MIDI Stop',d.status()['playing']==0)
 d.o.send(mido.Message('continue'));d.o.send(mido.Message('clock'));time.sleep(.08);check('USB MIDI Continue',d.status()['playing']==1)
 time.sleep(2.15);check('external clock loss stops after two seconds',d.status()['playing']==0)
 d.command(61,0,2);d.o.send(mido.Message('start'));d.o.send(mido.Message('clock'));time.sleep(.1);check('USB clock ignored when TRS clock selected (routing only)',d.status()['playing']==0)
 d.command(61,0,0);d.command(27,0,0)
 for b in range(14):
  before=d.patch();d.remote([1,(1<<b)&127,(1<<b)>>7]);time.sleep(.95)
  check('held help '+str(b),d.status()['help']==b)
  d.remote([1,0,0]);time.sleep(.12);check('release does not tap '+str(b),d.patch()==before);d.tap(6)
 final=d.status()
 for field in ['xruns','overloads','nonfinite','dropped']:check('no new '+field+' through clock/help tests',final[field]==end[field])
 report=dict(firmware_sha256=hashlib.sha256(Path('work/mutable-dreams-next/app/build/mutable-dreams-1.1.0-dev.fwsc').read_bytes()).hexdigest(),checks=checks,cycles=loops,messages=loops*18,baseline=base,after_midi=end,after_clock_help=final,clock=clock,audio=audio,physical_trs_tested=False)
 (r/'midi-long.json').write_text(json.dumps(report,indent=2))
finally:
 d.remote([1,0,0]);d.command(27,0,0);d.command(61,0,0);d.remote([4,1,32]);d.close();meter.stream.stop();meter.stream.close()
