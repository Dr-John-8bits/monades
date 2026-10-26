import sys,time,struct,json
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from hardware_bench import Device
r=Path('work/device-mellow');d=Device('X0X FM-1');checks=[]
def check(s,ok):assert ok,s;checks.append(s);print('PASS',s,flush=True)
def snap(obj):
 n=struct.unpack_from('<I',d.call(6,[obj]))[0]
 return b''.join(d.call(5,[off&127,off>>7]) for off in range(0,n,224))
def turn(role,n):d.remote([3,role,n+64]);time.sleep(.15)
try:
 d.command(27,0,0);d.command(61,0,0)
 check('named user Biome survives firmware update/restart',snap(18)==(r/'user-biome-16.raw').read_bytes())
 check('user sound bank survives firmware update/restart',snap(22)==(r/'user-tone-bank-4.raw').read_bytes())
 check('saved project survives restart byte-for-byte against pre-flash backup',snap(0)==Path('outputs/Mutable-Dreams-Private-Backup-2026-10-24/After-Bank-Test/FM1-Gespeichertes-Projekt.raw').read_bytes())
 d.tap(6)
 for _ in range(5):d.tap(5)
 turn(3,15);turn(5,1)
 check('saved own Biome recalls after reboot',d.patch()==(r/'user-biome-16.raw').read_bytes()[28:])
 # Real USB MIDI: channels and program addressing, control routing and clock.
 import mido
 d.command(20,0,64);d.command(45,0,208);d.command(45,1,160);before=d.status()['events']
 d.o.send(mido.Message('note_on',channel=0,note=48,velocity=90));d.o.send(mido.Message('note_on',channel=1,note=55,velocity=90));time.sleep(.15)
 after=d.status()['events'];check('USB MIDI channels independently trigger tracks 1/2',after[0]==before[0]+1 and after[1]==before[1]+1)
 for ch in [0,1]:d.o.send(mido.Message('note_off',channel=ch,note=48 if ch==0 else 55,velocity=0))
 d.o.send(mido.Message('control_change',channel=0,control=74,value=40));time.sleep(.1)
 check('USB CC74 edits selected channel filter',struct.unpack('<323i',d.patch())[32]==40*1000//127)
 d.o.send(mido.Message('control_change',channel=0,control=0,value=1));d.o.send(mido.Message('program_change',channel=0,program=80));time.sleep(.15)
 check('USB bank/program reaches Satin preset 208',struct.unpack('<323i',d.patch())[156]==208)
 d.command(61,0,1);d.o.send(mido.Message('start'))
 for _ in range(48):d.o.send(mido.Message('clock'));time.sleep(1/48)
 check('USB external clock starts transport',d.status()['playing']==1)
 time.sleep(2.2);check('clock-loss timeout stops transport',d.status()['playing']==0)
 d.command(61,0,0);d.command(27,0,0);d.command(20,0,73);d.tap(6)
 (r/'restart-current.json').write_text(json.dumps(dict(checks=checks,status=d.status()),indent=2))
finally:d.command(27,0,0);d.remote([4,1,0]);d.close()
