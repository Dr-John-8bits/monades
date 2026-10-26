import sys,time,json
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from hardware_bench import Device,Meter
r=Path('work/device-mellow');d=Device('X0X FM-1');meter=Meter();rows=[]
def prepare(models,quality=0):
 d.command(27,0,0);d.command(20,0,64)
 for t,m in enumerate(models):
  d.command_many([(c,t,v) for c,v in [(0,m),(12,0),(4,450),(10,36+t*7),(7,16),(6,16),(9,1000),(13,1),(3,1000),(28,0),(1,400),(2,280),(40,300),(41,700),(42,700),(43,2),(5,1000),(30,4),(46,800),(47,800),(56,600),(58,500)]])
 d.command_many([(51,0,900),(49,0,800),(50,0,850),(14,0,500),(14,1,0),(14,3,1000),(22,0,650),(24,0,850),(25,0,quality),(15,0,220),(34,0,24),(35,0,1)])
 import struct
 w=struct.unpack('<323i',d.patch());assert w[322]==900 and all(w[163+26*t]==800 and w[164+26*t]==800 for t in range(6))
def measure(name,seconds,animate=False,all_tracks=True):
 d.call(3);base=d.status();a=meter.snapshot();d.command(16,0,1);readings=[];issues=[];start=time.monotonic();last=0;changed=0
 while time.monotonic()-start<seconds:
  time.sleep(.5);s=d.status();readings.append(s);elapsed=time.monotonic()-start
  for k in ['xruns','overloads','nonfinite','dropped']:
   if s[k]!=base[k]:issues.append(k)
  if not s['playing']:issues.append('stopped')
  if animate and elapsed-changed>10:
   changed=elapsed;i=int(elapsed//10);d.tap([6,4,9,0,5][i%5]);d.command_many([(22,0,[100,190,200,210,300,500,1000][i%7]),(23,0,(i*131)%1001),(34,0,[-24,0,12,24][i%4]),(35,0,i%2)])
  if elapsed-last>30:
   last=elapsed;print('PROGRESS',name,round(elapsed),'s',round(s['callback_peak_us']/s['callback_budget_us']*100,2),'%',flush=True)
  if issues:break
 d.command(27,0,0);b=meter.snapshot();frames=b['frames']-a['frames'];energy=b['energy']-a['energy'];cpu=max(x['callback_peak_us']/x['callback_budget_us']*100 for x in readings)
 if cpu>=85:issues.append('reserve below 15%')
 if b['host_input_overflows']!=a['host_input_overflows']:issues.append('capture overflow')
 if energy<1e-8:issues.append('silent')
 if b['nonfinite']!=a['nonfinite']:issues.append('USB nonfinite')
 if frames<44100*(time.monotonic()-start)*.8:issues.append('USB capture incomplete')
 if all_tracks and any(z<=x for x,z in zip(base['events'],s['events'])):issues.append('not all six tracks triggered')
 row=dict(name=name,cpu_peak_percent=cpu,seconds=time.monotonic()-start,issues=sorted(set(issues)),rms=(energy/max(1,frames*2))**.5,baseline=base,final=s);rows.append(row);(r/'warm-current.json').write_text(json.dumps(rows,indent=2));print(json.dumps({k:row[k] for k in ['name','cpu_peak_percent','issues','rms']}),flush=True)
 return not issues
try:
 meter.stream.start();d.status();d.remote([4,0,16]);d.tap(6)
 for model,name,soft in [(1,'snare',8),(3,'fm',11),(5,'rings',10),(6,'harmonic',10)]:
  prepare([model]*3+[soft]*3);measure('three-'+name+'-plus-three-warm',10)
 for biome in range(65,77):
  d.command(27,0,0);d.command(20,0,biome);measure('warm-biome-'+str(biome),6,all_tracks=False)
finally:
 d.command(27,0,0);d.remote([4,1,0]);d.close();meter.stream.stop();meter.stream.close()
