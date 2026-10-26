"""Exact release-build smoke/stress qualification, preserving all personal data."""
from pathlib import Path
import sys,time,json,hashlib,subprocess,io,shutil
sys.path.insert(0,'work/mutable-dreams-release-1.1/tools')
from hardware_bench import Device,Meter
from device_editor import Editor
from dreams_backup import B,KIND
import fm1_install as I
import mido
q=Path('work/release-qualification');src=Path('work/mutable-dreams-release-1.1')
p=Path('outputs/Mutable-Dreams-Private-Backup-2026-10-25-Release-Label')
fw=src/'app/build/mutable-dreams-1.1.0.fwsc'
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
baseline=(p/'Banks-a.mddb').read_bytes();assert baseline==(p/'Banks-b.mddb').read_bytes()
expected=dict(B.parse(baseline,KIND)[1]);live=(p/'live.raw').read_bytes()
assert len(expected)==22 and len(live)==1292
assert json.loads((q/'release-source-comparison.json').read_text())['dsp_and_platform_unchanged']
route=q/'release-direct';route.mkdir(exist_ok=True)
for name in ([] if '--resume-after-install' in sys.argv else ['direct_upgrade.py','restore_after_direct.py']):
    code=(q/name).read_text().replace('work/mutable-dreams-next','work/mutable-dreams-release-1.1')
    code=code.replace("Path('work/release-qualification')", "Path('work/release-qualification/release-direct')")
    code=code.replace('Mutable-Dreams-Private-Backup-2026-10-25-Direct-Upgrade','Mutable-Dreams-Private-Backup-2026-10-25-Release-Label')
    code=code.replace('outputs/Mutable-Dreams-1.1-Mellow-Preview/Mutable-Dreams-1.1.0-dev.fwsc',str(fw))
    code=code.replace('427a34eedd2cc625b7833f3f953436bc1db1845f94cde695d8eed47332d09534',sha(fw))
    (route/name).write_text(code)
    with (route/(name+'.log')).open('w') as log:subprocess.run([sys.executable,str(route/name)],stdout=log,stderr=subprocess.STDOUT,check=True)
shutil.copy2(route/'direct-1.0-to-1.1.log',q/'install-release.log')
assert 'done: the FM-1 runs FM-1_710010100' in (q/'install-release.log').read_text()
prior=json.loads((route/'direct-upgrade.json').read_text())
assert prior['candidate_firmware_sha256']==sha(fw) and prior['all_22_original_objects_restored'] and not prior['restoration_pending']
if '--resume-after-install' in sys.argv:
    identity=subprocess.check_output([sys.executable,str(src/'base/tools/fm1_install.py'),'--info'],text=True)
    assert 'FM-1_710010100' in identity
    print('PASS resume: prior exact install, complete data restoration and current identity verified',flush=True)
print('PASS exact release installed',sha(fw),flush=True)
report=dict(pass_=False,firmware_sha256=sha(fw),installed=True,rows=[],source_comparison='release-source-comparison.json')
report['direct_1_0_upgrade']=json.loads((route/'direct-upgrade.json').read_text())
(q/'release-device.json').write_text(json.dumps(report,indent=2))
def backup(label):
    with (p/(label+'.log')).open('w') as log:B.backup(I.MidoBackend(),p/(label+'.mddb'),'X0X FM-1',log,KIND)
    assert dict(B.parse((p/(label+'.mddb')).read_bytes(),KIND)[1])==expected
backup('After-release-install');report['all_22_saved_objects_survived_install']=True
d=Editor();d.status=Device.status.__get__(d)
try:
    assert d.snapshot()==live;report['saved_project_exact']=True;report['after_migration_restore']=d.status()
finally:d.close()
d=Device('X0X FM-1');meter=Meter()
class Link:
    lost=False
    def send(self,msg):d.o.send(mido.Message.from_bytes(msg));return True
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
    d.call(3);before=d.status();a=meter.snapshot();d.command(16,0,1);start=time.monotonic();i=0;backed=False;last=0
    print('START',name,flush=True)
    while time.monotonic()-start<seconds:
        if stress:
            z=i%19+8192;d.call(10,[100,0,z&127,z>>7]);d.call(9)
            d.command_many([(22,0,[190,200,210,400,700,1000][i%6]),(23,0,(i*79)%1001),(41,i%6,300+(i*31)%600)])
        else:time.sleep(.12)
        i+=1;elapsed=time.monotonic()-start
        if stress and elapsed>seconds/2 and not backed:
            d.o.send(mido.Message.from_bytes(B.frame(B.BU_REQ,b'MDDB')))
            flags,objects=B.parse(B.receive(Link(),io.StringIO(),KIND),KIND)
            assert flags&1 and dict(objects)==expected;backed=True
        s=d.status()
        if elapsed-last>20:last=elapsed;print('PROGRESS',name,round(elapsed),'seconds',flush=True)
    d.command(27,0,0);after=d.status();b=meter.snapshot()
    faults=[k for k in ['xruns','overloads','nonfinite','dropped','quality_reductions'] if before[k]!=after[k]]
    if any(a[k]!=b[k] for k in ['host_input_overflows','nonfinite']):faults.append('USB capture error')
    frames=b['frames']-a['frames'];energy=b['energy']-a['energy']
    if frames<seconds*44100*.9 or energy<1e-8:faults.append('incomplete or silent audio')
    peak=after['callback_peak_us']/after['callback_budget_us']*100
    if peak>=85:faults.append('less than 15 percent callback reserve')
    if stress and any(y<=x for x,y in zip(before['events'],after['events'])):faults.append('untriggered track')
    row=dict(name=name,seconds=time.monotonic()-start,peak_callback_percent=peak,before=before,after=after,backup_while_playing=backed,rms=(energy/max(1,frames*2))**.5,faults=faults)
    report['rows'].append(row);(q/'release-device.json').write_text(json.dumps(report,indent=2));print('RESULT',name,round(peak,2),faults,flush=True);assert not faults
try:
    meter.stream.start();d.status();d.remote([4,0,16]);d.command(61,0,0)
    prepare([6]*6);measure('Six Harmonic + effects + editor + backup',60,True)
    prepare([9,5,4,11,8,7]);d.command(40,0,500);measure('Six mixed engines + effects + editor + backup',60,True)
    for ident,name in [(67,'Mellow Canopy'),(73,'Satin River'),(75,'Velour Dawn')]:
        d.command(20,0,ident);measure(name,25,False)
finally:
    try:d.command(27,0,0);d.command(61,0,0);d.remote([1,0,0]);d.remote([2,0,0,0,0]);d.remote([4,1,32]);d.remote([5,0])
    finally:d.close();meter.stream.stop();meter.stream.close()
    d=Editor()
    try:d.upload(live);assert d.snapshot()==live;d.ui(100,0,0)
    finally:d.close()
    report['user_live_restored']=True;(q/'release-device.json').write_text(json.dumps(report,indent=2))
# One actual User Biome save/load with precisely the existing name and payload.
# No invented content and no full-bank restore/confirmation bypass.
obj=next((o for o in range(3,19) if len(expected[o])==1320),None)
assert obj is not None, 'No occupied slot; do not overwrite an unknown bank'
d=Editor();d.status=Device.status.__get__(d)
try:
    d.bank(0,obj-3,0,1);assert d.snapshot()==expected[obj][28:]
    before=d.status();name=expected[obj][4:28].split(b'\0')[0].decode('ascii')
    d.bank(0,obj-3,0,3,name);after=d.status();assert d.snapshot()==expected[obj][28:]
    report['identical_user_biome_resave']=dict(passed=True,xrun_delta=after['xruns']-before['xruns'])
finally:
    d.upload(live);assert d.snapshot()==live;d.ui(100,0,0);d.close()
backup('After-release-tests');report['all_22_saved_objects_after_tests_exact']=True
d=Device('X0X FM-1')
try:
    d.remote([1,0,0]);d.remote([2,0,0,0,0]);d.remote([4,1,32]);d.remote([5,0]);report['final_status']=d.status()
finally:d.close()
assert not report['final_status']['playing'] and report['final_status']['page']==0
report['pass_']=True;report['device_free']=True
(q/'release-device.json').write_text(json.dumps(report,indent=2))
(p/'SHA256SUMS.txt').write_text(''.join(f'{sha(x)}  {x.name}\n' for x in sorted(p.iterdir()) if x.is_file() and x.name!='SHA256SUMS.txt'))
print('PASS release build; all 22 saved objects and unsaved live exact, HOME/stopped/physical controls released.',flush=True)
