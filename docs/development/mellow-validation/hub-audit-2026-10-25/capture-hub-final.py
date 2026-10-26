import sys,time,json,wave,hashlib
from pathlib import Path
import numpy as np,sounddevice as sd
sys.path.insert(0,'work/mutable-dreams-next/tools')
from hardware_bench import Device
out=Path('outputs/Mutable-Dreams-1.1-Mellow-Preview/Audio-FM1-Audit');out.mkdir(exist_ok=True)
d=Device('X0X FM-1');rows=[]
firmware_sha256=hashlib.sha256(Path('work/device-mellow/plectrum-before-organic-glass.fwsc').read_bytes()).hexdigest()
devices=sd.query_devices();idx=next(i for i,x in enumerate(devices) if 'Mutable Dreams' in x['name'] and x['max_input_channels']==2)
try:
 d.status();d.tap(6);d.remote([4,0,16]);d.command(27,0,0)
 for id,name in [(67,'Mellow-Canopy'),(73,'Satin-River'),(75,'Velour-Dawn')]:
  d.command(20,0,id);d.call(3);before=d.status();chunks=[];overflow=[]
  (out/(name+'.mutable-dreams.json')).write_text(json.dumps(dict(format='mutable-dreams-patch',version=1,firmware='1.1-dev',name=name,patch=list(d.patch())),indent=2))
  def cb(x,n,t,status):chunks.append(x.copy());overflow.append(bool(status.input_overflow))
  with sd.InputStream(device=idx,samplerate=44100,channels=2,dtype='float32',blocksize=1024,callback=cb):
   d.command(16,0,1);time.sleep(28);d.command(16,0,0);time.sleep(2)
  a=np.concatenate(chunks);assert np.isfinite(a).all() and not any(overflow)
  peak=float(np.max(np.abs(a)));rms=float(np.sqrt(np.mean(a*a)));assert peak<1 and rms>1e-5
  # Retain the exact captured level. No loudness enhancement or other processing.
  file=out/(name+'.wav')
  pcm=np.clip(np.rint(a*8388608),-8388608,8388607).astype('<i4').view(np.uint8).reshape(-1,4)[:,:3].tobytes()
  with wave.open(str(file),'wb') as f:f.setparams((2,3,44100,len(a),'NONE','not compressed'));f.writeframes(pcm)
  after=d.status();assert all(before[k]==after[k] for k in ['xruns','overloads','dropped','nonfinite'])
  rows.append(dict(biome=id,name=name,firmware_sha256=firmware_sha256,source=devices[idx]['name'],container='24-bit PCM WAV, 44100 Hz stereo; source precision as supplied by USB driver',frames=len(a),peak=peak,rms=rms,host_overflows=sum(overflow),sha256=hashlib.sha256(file.read_bytes()).hexdigest(),cpu_peak_percent=after['callback_peak_us']/after['callback_budget_us']*100,status=after));print('CAPTURED',name,peak,rms,flush=True)
 (out/'Capture-Report.json').write_text(json.dumps(rows,indent=2))
finally:
 d.command(27,0,0);d.command(20,0,73);d.tap(6);d.remote([4,1,32]);d.close()
