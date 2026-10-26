import sys,time,struct,json,hashlib
from pathlib import Path
sys.path.insert(0,'work/mutable-dreams-next/tools')
from hardware_bench import Device
out=Path('work/device-mellow');d=Device('X0X FM-1');checks=[]
def check(name,ok):
 assert ok,name
 checks.append(name);print('PASS',name,flush=True)
def turn(role,n):d.remote([3,role,n+64]);time.sleep(.12)
def snapshot(obj):
 raw=d.call(6,[obj]);size=struct.unpack_from('<I',raw)[0]
 data=b''.join(d.call(5,[off&127,off>>7]) for off in range(0,size,224))
 assert len(data)==size
 return data
def banks():
 d.tap(6)
 for _ in range(5):d.tap(5)
 check('User Banks reachable using GLO',d.status()['page']==18)
def load():turn(5,1)
def w():return struct.unpack('<323i',d.patch())
def name(s):
 turn(6,-63);old='MY BIOME'
 for i,c in enumerate(s):
  before=ord(old[i]) if i<len(old) else 32
  diff=ord(c)-before
  while diff:
   step=max(-63,min(63,diff));turn(2,step);diff-=step
  if i<len(s)-1:turn(6,1)
try:
 d.command(27,0,0);d.command(61,0,0);d.status()
 # Never overwrite an occupied user slot to make a test fit.
 for obj in [18,22]:
  try:
   raw=snapshot(obj)
  except RuntimeError as e:
   if 'status 3' not in str(e):raise
  else:raise RuntimeError('Test slot already occupied: '+str(obj))
 d.command(20,0,73);d.command(15,0,73)
 d.command(52,1,2);d.command(7,1,8)
 for i,n in enumerate([0,-1,7,0,12,-1,7,3]):d.command_many([(59,1,i),(60,1,n)])
 banks();turn(3,15);name('MY SATIN BIOME');d.tap(7);time.sleep(.2)
 desired=d.patch();stored=snapshot(18)
 check('named Biome saves complete patch and custom sequence',stored[28:]==desired and stored[4:28].split(b'\0')[0]==b'MY SATIN BIOME')
 (out/'user-biome-16.raw').write_bytes(stored)
 d.command(20,0,64);load();check('custom Biome recalls all six tracks exactly',d.patch()==desired)
 d.command(15,0,74);d.tap(7)
 check('first SAVE protects occupied Biome slot',snapshot(18)==stored)
 d.tap(7);time.sleep(.2);check('second SAVE confirms overwrite',snapshot(18)[28:]==d.patch())
 # Restore musical test scene and exercise storing under playback guard.
 d.command(15,0,73);d.tap(7);d.tap(7);time.sleep(.2)
 expected=snapshot(18);check('restored named Biome matches baseline',expected==stored)
 d.command(16,0,1);d.tap(7);d.command(27,0,0);check('SAVE refused during playback',snapshot(18)==stored)
 # Tone sound state vs pattern/mixer state.
 d.command_many([(45,0,211),(54,0,350),(56,0,400),(57,0,600),(58,0,200)])
 turn(0,-63);turn(4,1);turn(3,63);d.tap(7);time.sleep(.2)
 tone=snapshot(22);(out/'user-tone-bank-4.raw').write_bytes(tone)
 check('user tone 64 stored in last packed bank',len(tone)==3464 and struct.unpack_from('<I',tone,4)[0]&(1<<15))
 saved=w();d.command_many([(45,0,160),(54,0,0),(56,0,0),(57,0,0),(58,0,0),(10,0,43),(4,0,370),(5,0,220),(46,0,111),(47,0,222),(6,0,3),(52,0,2)])
 before=w();load();after=w()
 fields=[0,1,2,3,14,17,18,19,20,21]
 check('sound recall restores oscillator/filter/envelope',[after[14+x] for x in fields]==[saved[14+x] for x in fields])
 # Extension starts at word 163, 26 words per track.
 check('sound recall restores Glide Tides Warps',[after[163+x] for x in [4,6,7,8]]==[saved[163+x] for x in [4,6,7,8]])
 keep=[4,5,6,7,8,9,10,11,12,13,15,16]
 check('sound recall preserves mix notes and sequencing',[after[14+x] for x in keep]==[before[14+x] for x in keep] and [after[163+x] for x in [0,1,2,3]]==[before[163+x] for x in [0,1,2,3]])
 # Hold every physical function key. Release must only leave help open.
 d.tap(6)
 for b in range(14):
  before=d.patch();d.remote([1,(1<<b)&127,(1<<b)>>7]);time.sleep(.95)
  check('hold help button '+str(b),d.status()['help']==b)
  d.remote([1,0,0]);time.sleep(.1);check('release does not tap button '+str(b),d.patch()==before);d.tap(6)
 (out/'bank-checks.json').write_text(json.dumps(dict(checks=checks,status=d.status()),indent=2))
finally:
 d.remote([1,0,0]);d.command(27,0,0);d.remote([4,1,0]);d.close()
