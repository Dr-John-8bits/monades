import sys,time,json,wave,hashlib
from pathlib import Path
import numpy as np,sounddevice as sd
sys.path.insert(0,'work/mutable-dreams-release-1.1/tools')
from hardware_bench import Device
from device_editor import Editor
r=Path('work/release-qualification/release-audio');p=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Release-Label')
out=Path('outputs/Mutable-Dreams-1.1-Release-Media/Audio-FM1');out.mkdir(parents=True,exist_ok=True)
assert json.loads((r.parent/'release-device.json').read_text())['all_22_saved_objects_after_tests_exact']
fw=hashlib.sha256(Path('work/mutable-dreams-release-1.1/app/build/mutable-dreams-1.1.0.fwsc').read_bytes()).hexdigest()
assert fw=='8e039bb0e9681fbbb0504e9b7d251895958fd88ae97f11ff0342b4aef38143d2'
devices=sd.query_devices();idx=next(i for i,x in enumerate(devices) if 'Mutable Dreams' in x['name'] and x['max_input_channels']==2)
rows=[];d=Device('X0X FM-1')
try:
 d.status();d.command(27,0,0);d.command(61,0,0);d.remote([4,0,16]);d.tap(6)
 for ident,name in [(67,'Mellow-Canopy'),(73,'Satin-River'),(75,'Velour-Dawn'),(26,'Dub-Estuary'),(1,'Soft-Machines'),(51,'Clay-Ensemble'),(73,'Satin-Grain-Size-Sweep')]:
  d.command(20,0,ident);sweep='Sweep' in name
  if sweep:d.command_many([(14,3,650),(22,0,200)])
  patch=d.patch();(out/(name+'.mutable-dreams.json')).write_text(json.dumps(dict(format='mutable-dreams-patch',version=1,firmware='1.1',name=name,patch=list(patch)),indent=2))
  d.call(3);before=d.status();chunks=[];flags=[];events=[]
  def cb(x,n,t,status):chunks.append(x.copy());flags.append(str(status) if status else '')
  print('RECORD',name,flush=True)
  with sd.InputStream(device=idx,samplerate=44100,channels=2,dtype='float32',blocksize=1024,callback=cb):
   d.command(16,0,1);start=time.monotonic()
   if sweep:
    for value in [200,210,500,1000]:
     d.command(22,0,value);events.append(dict(seconds=round(time.monotonic()-start,3),grain_percent=value/10));time.sleep(10)
   else:time.sleep(35)
   d.command(16,0,0);time.sleep(3)
  a=np.concatenate(chunks);after=d.status();peak=float(np.max(np.abs(a)));rms=float(np.sqrt(np.mean(a*a)))
  assert np.isfinite(a).all() and not any(flags),'Capture nonfinite/host status'
  assert peak<1 and rms>1e-5,'Clipping or silence'
  assert len(a)>=44100*(42 if sweep else 37),'Incomplete recording'
  assert all(before[k]==after[k] for k in ['xruns','overloads','dropped','nonfinite','quality_reductions']),name
  assert after['callback_peak_us']/after['callback_budget_us']<.85,name+' timing reserve'
  file=out/(name+'.wav');pcm=np.clip(np.rint(a*8388608),-8388608,8388607).astype('<i4').view(np.uint8).reshape(-1,4)[:,:3].tobytes()
  with wave.open(str(file),'wb') as f:f.setparams((2,3,44100,len(a),'NONE','not compressed'));f.writeframes(pcm)
  row=dict(name=name,biome=ident,firmware_sha256=fw,source=devices[idx]['name'],seconds=len(a)/44100,container='24-bit PCM WAV, 44.1kHz stereo; precision limited to actual USB source',master_override=2048,processing='None: original captured level, no normalization, effects are from the firmware',peak=peak,rms=rms,dc=float(np.mean(a)),host_flags=[x for x in flags if x],file_sha256=hashlib.sha256(file.read_bytes()).hexdigest(),events=events,before=before,after=after)
  rows.append(row);(out/'Capture-Report.json').write_text(json.dumps(rows,indent=2));print('PASS',name,'peak',round(peak,4),'callback',round(after['callback_peak_us']/after['callback_budget_us']*100,2),'%',flush=True)
finally:
 try:d.command(27,0,0);d.command(61,0,0);d.remote([1,0,0]);d.remote([2,0,0,0,0]);d.remote([4,1,32]);d.remote([5,0])
 finally:d.close()
 d=Editor()
 try:d.upload((p/'live.raw').read_bytes());assert d.snapshot()==(p/'live.raw').read_bytes();d.ui(100,0,0)
 finally:d.close()
 (r/'capture-restored.json').write_text(json.dumps(dict(latest_post_editor_live_restored=True,saved_objects_not_written=True,clips_completed=len(rows)),indent=2))
print('PASS all clips captured; latest user live state restored; no saved banks written.',flush=True)
