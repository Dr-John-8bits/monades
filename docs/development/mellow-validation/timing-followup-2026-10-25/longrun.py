import sys,time,json,hashlib,struct,io
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from hardware_bench import Device,Meter
from dreams_backup import B,KIND
import mido
out=Path('work/timing-audit');d=Device('X0X FM-1');meter=Meter();rows=[]
firmware=hashlib.sha256(Path('work/mutable-dreams-next/app/build/mutable-dreams-1.1.0-dev.fwsc').read_bytes()).hexdigest()
baseline=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Timing/Banks-a.mddb').read_bytes();expected=B.parse(baseline,KIND)[1]
class Link:
 lost=False
 def send(self,p):d.o.send(mido.Message.from_bytes(p));return True
 def read(self,timeout):
  end=time.monotonic()+timeout
  while time.monotonic()<end:
   for m in d.i.iter_pending():
    if m.type=='sysex':return bytes(m.bytes())
   time.sleep(.001)
  return None

def prepare(models):
 d.command(27,0,0);d.command(20,0,64)
 for t,m in enumerate(models):
  d.command_many([(c,t,v) for c,v in [(0,m),(12,0),(4,450),(10,36+t*7),(7,16),(6,16),(9,1000),(13,1),(3,1000),(28,0),(1,400),(2,280),(40,300),(41,700),(42,700),(43,2),(5,1000),(30,4),(46,800),(47,800),(56,600),(58,500)]])
 d.command_many([(51,0,900),(49,0,800),(50,0,850),(14,0,500),(14,1,0),(14,3,1000),(22,0,650),(24,0,850),(25,0,0),(15,0,220),(34,0,24),(35,0,1)])

def measure(name,seconds,stress):
 d.call(3);start_status=d.status();a=meter.snapshot();d.command(16,0,1);start=time.monotonic();last=0;backed=False;values=[];i=0
 while time.monotonic()-start<seconds:
  if stress:
   pg=i%19;z=pg+8192;d.call(10,[100,0,z&127,z>>7]);d.call(9)
   d.command_many([(22,0,[190,200,210,400,700,1000][i%6]),(23,0,(i*79)%1001),(41,i%6,300+(i*31)%600)])
  else:time.sleep(.12)
  i+=1
  elapsed=time.monotonic()-start
  if stress and elapsed>seconds/2 and not backed:
   d.o.send(mido.Message.from_bytes(B.frame(B.BU_REQ,b'MDDB')))
   data=B.receive(Link(),io.StringIO(),KIND);flags,objects=B.parse(data,KIND);assert flags&1 and objects==expected
   backed=True;print('PASS streaming backup during',name,flush=True)
  s=d.status();values.append(s)
  if elapsed-last>20:last=elapsed;print('PROGRESS',name,round(elapsed),round(s['callback_peak_us']/s['callback_budget_us']*100,2),flush=True)
 d.command(27,0,0);b=meter.snapshot();issues=[]
 for k in ['xruns','overloads','nonfinite','dropped','quality_reductions']:
  if values[-1][k]!=start_status[k]:issues.append(k)
 if any(b[k]!=a[k] for k in ['host_input_overflows','nonfinite']):issues.append('USB capture')
 frames=b['frames']-a['frames'];energy=b['energy']-a['energy'];cpu=max(s['callback_peak_us']/s['callback_budget_us']*100 for s in values)
 if energy<1e-8:issues.append('silent')
 if frames<44100*seconds*.9:issues.append('capture incomplete')
 if cpu>=85:issues.append('less than 15 percent callback reserve')
 if stress and any(z<=x for x,z in zip(start_status['events'],values[-1]['events'])):issues.append('untriggered track')
 row=dict(name=name,firmware_sha256=firmware,seconds=time.monotonic()-start,cpu_peak_percent=cpu,issues=issues,rms=(energy/max(1,2*frames))**.5,backup_while_playing=backed,baseline=start_status,final=values[-1]);rows.append(row);(out/'long-audio-stress.json').write_text(json.dumps(rows,indent=2));print('RESULT',name,cpu,issues,flush=True);assert not issues
try:
 meter.stream.start();d.status();d.remote([4,0,16])
 prepare([6]*6);measure('six-harmonic-live-editor-and-backup',300,True)
 prepare([9,5,4,11,8,7]);d.command(40,0,500);measure('mixed-six-live-editor-and-backup',300,True)
 for id,name in [(67,'Mellow Canopy'),(73,'Satin River'),(75,'Velour Dawn')]:
  d.command(20,0,id);measure(name,30,False)
finally:
 d.command(27,0,0);d.remote([4,1,32]);d.close();meter.stream.stop();meter.stream.close()
