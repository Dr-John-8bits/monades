#!/usr/bin/env python3
"""Authoring source for the factory atlas. Deterministic, no runtime random bank.
Generated C tables live in flash; JSON supplies the browser's matching labels.
Tone recall intentionally preserves note, mix, sends, rhythm and mute.
"""
from pathlib import Path
import json
R=Path(__file__).resolve().parents[1]
fields='model tone harm decay level send hits length rotate prob note pan mute speed attack ratchet accent shape cutoff resonance filter breathe'.split()
tone_fields='model tone harm decay attack shape cutoff resonance filter breathe'.split()
names=[
'Felt Foot|Deep Pillow|Round Sub|Moss Kick|Soft Knock|Dub Heart|Clay Drum|Warm Pulse|Low Pebble|Thud Room|Rubber Step|Satin Kick|Stone Floor|Dust Punch|Long Earth|Tight Seed',
'Brush Skin|Paper Snare|Muted Rim|Sand Brush|Velvet Snap|Dry Twig|Soft Clap|Dust Snare|Thin Shell|Wood Snare|Loose Skin|Grain Snap|Rain Stick|Low Rattle|Warm Wire|Short Ember',
'Silk Hat|Mist Tick|Dry Leaf|Closed Dust|Shaker Sand|Open Air|Fine Rain|Soft Metal|Leaf Shake|Glass Dust|Tiny Bell|Loose Seeds|Steam Hat|Warm Ride|Grain Rust|Short Hiss',
'Pure Water|Sine Pearl|Wool Bass|Glass Drop|Soft Tine|Rubber Bell|Warm Reed|Amber Keys|Low Orbit|Velvet FM|Copper Bell|Hollow Chime|Dream Marimba|Deep Bloom|Moon Tone|Slow Glass',
'Round Reed|Wool Pad|Satin Bass|Soft Saw|Warm Strings|Hollow Flute|Dust Organ|Muted Pluck|Slow Brass|Cedar Pad|Moss Pulse|Silk Lead|Low Current|Warm Fifth|Cloud Reed|Felt Keys',
'Wood Bowl|Warm Wire|Clay Bell|Moss String|Glass Pool|Soft Steel|Hollow Log|Deep Vessel|Copper Rain|Silk String|Small Stone|Low Gong|Paper Plate|Ice Bell|Amber Chime|Felt Mallet',
'Sine Lantern|Warm Organ|Odd Velvet|Soft Spectrum|Hollow Pipe|Amber Pad|Glass Choir|Low Partial|Silk Harmonic|Felt Organ|Slow Aurora|Moss Chord|High Mist|Bronze Bloom|Deep Spectrum|Pure Harmonic',
'Minor Cloud|Major Dawn|Suspended Air|Open Fifth|Velvet Choir|Sunlit Voices|Floating Sus|Deep Fifth|Minor Moss|Major Silk|Suspended Dew|Wide Horizon|Night Voices|Warm Meadow|Slow Halo|Low Shelter']
# Individual contour tables, rather than 128 copies with arbitrary names.
tones=[]
t=[55,90,140,205,280,340,410,480,100,170,260,350,460,560,70,220]
h=[60,150,280,420,560,700,840,950,220,370,510,640,780,900,100,320]
d=[250,700,420,180,550,820,330,620,920,480,200,760,380,600,1000,120]
a=[0,0,0,0,100,0,160,0,450,300,0,0,80,650,800,350]
for model,row in enumerate(names):
 for j,name in enumerate(row.split('|')):
  attack=a[j] if model in [3,4,6,7] else 0
  shape=([500,500,490,510,485,520,500,505,495,500,530,470,510,500,500,515][j] if model==3 else 750 if model==6 else 80+j*34 if model==4 else 90+j*31)
  tone=t[j] if model!=7 else 100+j*28
  harm=h[j] if model!=7 else [0,300,600,900][j%4]
  cutoff=([660,620,570,720,690,740,680,710,550,640,760,630,700,590,610,730][j])
  if model==2:cutoff=min(950,cutoff+170)
  if model==0:cutoff-=50
  tones.append(dict(id=len(tones),name=name,model=model,tone=tone,harm=harm,decay=d[j],attack=attack,shape=shape,cutoff=cutoff,resonance=35+(j%5)*45,filter=3 if j in [3,7,12,15] else 2 if j in [2,8,9,13] else 1,breathe=(180+(j%4)*80) if attack else (70 if j%3==0 else 0)))
# Keep pitched presets on musical oscillator-ratio plateaus. VA's original
# HARMONICS control spans continuously between intervals; arbitrary values
# can detune the second oscillator by several non-scale semitones.
for x in tones:
 j=x['id']%16;m=x['model']
 if m==3:x.update(harm=[234,234,477,649,477,750,234,649,477,234,750,649,477,234,750,649][j],shape=500,tone=min(x['tone'],380))
 if m==4:x.update(harm=[500,500,500,622,744,500,622,500,744,500,622,500,500,622,744,500][j])
 if m==6:x.update(tone=[0,25,45,60,20,35,75,0,50,15,40,65,85,55,5,0][j],shape=800)
 if m==5:x.update(decay=min(x['decay'],650))
 x['resonance']=0 if m>=3 else min(x['resonance'],70)
 if m==5:x['harm']=[260,280,310,270,300,340,265,285,330,275,260,320,295,350,315,270][j]
 if m in [3,4,6,7]:x['cutoff']=min(930,x['cutoff']+130)
 if m==7:x['shape']=min(x['shape'],220)
family_names=['Mist & Atmosphere','Drone & Depth','Resonance & Wood','Dub & Techno','Broken Pulses','Organic Cycles','Rhythm Laboratory','Light & Melody']
family_short=['MIST','DRONE','RESONANCE','TECHNO','BROKEN','ORGANIC','PERCUSSION','MELODY']
biome_names=[
'Velvet Mist|Pearl Lagoon|Amber Fog|Soft Canopy|Silver Marsh|Cloud Atlas|Still Water|Moon Meadow',
'Deep Tundra|Low Horizon|Slow Monolith|Submerged Sun|Midnight Soil|Ancient Air|Blue Cavern|Distant Shelter',
'Resonant Field|Woodland Bells|Clay Basin|Copper Grove|Glass River|Moss Resonance|Hollow Valley|Stone Chimes',
'Soft Machines|Dub Estuary|Velvet Circuit|Warm Current|Basalt Steps|Night Transit|Copper Motor|Soft Pressure',
'Broken Fern|Odd Islands|Loose Gravity|Skipping Stones|Fracture Bloom|Tilted Rain|Uneven Tide|Polyrhythm Bay',
'Root Network|Seed Dispersal|Rainforest Dew|Coral Pulse|Mycelium Drift|Branching Paths|Tidal Roots|Living Sand',
'Six Kicks|Six Brushes|Six Leaves|Clay Ensemble|Skin And Stone|Dust Ritual|Low Drum Circle|Wooden Clocks',
'Sunlit Path|Tiny Constellation|Amber Arpeggio|Open Clearing|Glass Orchard|Quiet Lanterns|Pastel Stream|Morning Orbit']
# Authored engine arrangements by musical family. Each variation changes chord,
# articulation, rhythmic topology and tape behaviour, not just a random seed.
arrangements=[
[7,6,5,4,3,7],[7,4,6,7,3,5],[5,5,3,6,5,4],[0,2,1,3,5,0],
[0,1,2,5,3,4],[5,7,2,6,3,5],[0,0,0,0,0,0],[3,6,7,5,4,3]]
roots=[0,2,7,5,9,3,10,0]; scales=[1,3,4,2,5,7,1,3]
biomes=[]
for fam,row in enumerate(biome_names):
 for j,name in enumerate(row.split('|')):
  tempo=([76,64,82,58,72,88,60,68][j] if fam==0 else [48,56,42,64,52,60,46,70][j] if fam==1 else [84,92,72,104,66,88,110,78][j] if fam==2 else [126,118,130,122,136,128,114,140][j] if fam==3 else [112,138,96,124,146,108,132,116][j] if fam==4 else [78,96,86,104,70,92,118,82][j] if fam==5 else [110,96,132,108,124,140,92,120][j] if fam==6 else [92,108,84,116,100,76,124,96][j])
  b=dict(name=name,family=fam,tempo=tempo,root=roots[j],scale=scales[j],seed=0xD4EA125+fam*7919+j*101,macro=[500,250+j*65,420+(j%4)*35,750 if fam==1 else 650 if fam==0 else 500 if fam in [2,5,7] else 200+(j%4)*65],locked=1,freeze=0,grain_size=450+(j%5)*100,grain_pos=150+j*90,feedback=300+(j%5)*75,quality=0,transpose=0,swing=(j%4)*100 if fam in [3,4,6] else 0,grain_pitch=[0,0,12,-12,7,0,-5,12][j],reverse=int(j in [3,6] and fam!=3),duck_source=0 if fam in [3,4] else -1,duck_amount=450 if fam in [3,4] else 0,duck_release=350,tracks=[],tones=[])
  for i,model in enumerate(arrangements[fam]):
   if fam==6:model=([0]*6 if j==0 else [1]*6 if j==1 else [2]*6 if j==2 else [0,1,5,2,5,0] if j==3 else [0,1,2,5,1,5]) [i]
   if fam==0 and j==0:ix=[116,96,80,65,48,112][i]
   else:ix=model*16+(j*3+i*5+fam)%16
   tone=tones[ix];track=dict.fromkeys(fields,0);track.update({k:tone[k] for k in tone_fields})
   track.update(level=500 if fam not in [3,4,6] else 580,send=700 if fam in [0,1] else 0 if fam in [3,4] and i==0 else 450,note=([36,55,48,60,67,36][i] if fam in [3,4,6] else [36,43,48,55,60,67][i] if fam==1 else [48,55,60,63,67,72][i]),pan=200+i*120,prob=1000 if i==0 and fam in [3,4] else 720+(j%4)*70,length=16 if fam in [3,6,7] else [13,16,15,11,17,19][(i+j)%6],speed=1 if fam in [3,4,6,7] else 4+(i+j)%4 if fam==1 else 2+(i+j)%3,ratchet=2 if fam==4 and (i+j)%3==0 else 1,accent=350 if fam in [3,4,6] else 120)
   track['hits']=([4,8+j%4,2,3+j%3,3,2][i] if fam==3 else [3,5,7,4,2,5][(i+j)%6] if fam==4 else 1 if fam==1 else 2+(i+j)%4)
   track['rotate']=(i+j*2)%track['length']
   if fam in [0,1] and model in [3,4,6,7]:track.update(attack=250+j*65,decay=800+j*25,cutoff=520+(i%3)*45,breathe=200+j*60)
   if fam==6:track['note']=30+i*5+j*2
   if i==0:track.update(rotate=0,prob=1000) # Audible anchor immediately when a Biome starts.
   b['tracks'].append(track);b['tones'].append(ix if all(track[k]==tone[k] for k in tone_fields) else -1)
  biomes.append(b)
# Keep the original four start indices and sounds for existing control tests/users.
order=[0,24,48,16]+[i for i in range(64) if i not in [0,24,48,16]]
biomes=[biomes[i] for i in order]
for bid in range(4):
 b=biomes[bid];b.update(tempo=126 if bid==1 else 76,seed=0xD4EA125,root=0,scale=1,macro=[500,400,500,220 if bid==1 else 650],grain_size=650,grain_pos=350,feedback=450,swing=160 if bid==1 else 0,grain_pitch=0,reverse=0,duck_source=0 if bid==1 else -1,duck_amount=450 if bid==1 else 0)
 models=[[7,6,5,4,3,7],[0,2,1,3,5,0],[0]*6,[5]*6][bid]
 for i,model in enumerate(models):
  b['tracks'][i]=dict(zip(fields,[model,100+i*20 if bid==0 else 250+i*70,300,350 if bid==1 else 800,600 if bid==1 else 500,(0 if i==0 else 250) if bid==1 else 650,(4 if i==0 else 9 if i==1 else 2 if i==2 else 3) if bid==1 else i+2 if bid==2 else 2+i%3,16 if bid==1 else 13+i%4,i,850,(36 if i in [0,5] else [48,55,60,63,67,72][i]) if bid==1 else 33+i*3 if bid==2 else [48,55,60,63,67,72][i],200+i*120,0,1 if bid==1 else 2+i%3,250 if bid==0 and model in [3,4,6,7] else 0,1,300 if bid==1 else 0,500 if model==3 else 750 if model==6 else 150,590 if bid==0 else 850,100,1 if bid==0 else 0,220 if bid==0 else 0]))
  b['tones'][i]=-1
# Musical curation pass: one tonal centre, deliberate roles, less competing
# random motion, and a clearly audible rhythmic anchor. IDs remain stable.
for bid,b in enumerate(biomes):
 fam=b['family'];j=bid%8;root=b['root'];major=b['scale'] in [2,5];third=4 if major else 3
 b['macro']=[500,60+(j%4)*35,500,360 if fam in [0,1] else 280 if fam in [2,5,7] else 140]
 b.update(grain_pitch=12 if fam in [0,1,7] and j==4 else 0,reverse=0,feedback=180+(j%4)*35,grain_pos=180+(j%4)*70,grain_size=650+(j%3)*100,locked=1)
 if fam in [3,4,6]:b['swing']=60+(j%3)*60
 for i,tr in enumerate(b['tracks']):
  m=tr['model'];tr.update(pan=[500,340,660,220,780,500][i],ratchet=1,prob=1000 if i==0 else 900 if fam in [0,1,2,5,7] else 1000)
  if m>=3:
   tr['note']=(36 if fam==1 and m in [3,4] and i==1 else 48)+root+[0,7,12,third,19,12][i]
   if m==3:tr.update(harm=[234,477,649,234,750,477][i],shape=500,tone=min(tr['tone'],300))
   if m==4:tr.update(harm=500 if i%2 else 622,shape=min(tr['shape'],450))
   if m==5:tr.update(harm=265+(i%3)*15,tone=min(tr['tone'],420),decay=600 if fam in [2,5] else 460)
   if m==6:tr.update(tone=15+(i%3)*10,shape=800)
   if m==7:tr.update(note=48+root+(12 if i==5 else 0),harm=300 if major else 0,shape=120)
   tr.update(filter=1,cutoff=720+(i%3)*55,resonance=0,breathe=100+(j%3)*60)
  if fam in [0,1]:
   tr.update(level=[520,250,270,230,250,190][i],send=[500,350,450,400,450,450][i],length=16,hits=[1,2,2,1,3,1][i] if fam==0 else 1,rotate=[0,4,8,3,6,12][i],speed=[2,2,2,4,2,4][i] if fam==0 else [4,4,3,4,4,8][i])
   if m in [3,4,6,7]:tr.update(attack=250 if bid==0 else 300+(j%3)*100,decay=850 if fam==1 else 740)
  elif fam in [3,4]:
   tr.update(length=16,speed=1,level=[660,290,370,310,240,240][i],hits=([4,8,2,3,2,1] if fam==3 else [3,2,7,3,2,3])[i],rotate=([0,1,4,2,6,12] if fam==3 else [0,4,1,3,6,9])[i],accent=200,send=0 if i==0 else 180)
   if m==0:tr.update(note=24+root,decay=420 if i==0 else 230,tone=180,harm=100)
   if m==1:tr.update(note=48+root,decay=250,tone=300,harm=400)
   if m==2:tr.update(note=72,decay=180,tone=340,level=260)
   if m in [3,4,6,7]:tr.update(attack=0,decay=330)
  elif fam==6:
   tr.update(length=16,speed=1,level=[630,280,270,230,210,200][i],hits=[4,2,8,3,2,1][i],rotate=[0,4,1,3,6,12][i],send=70,decay=280+(i%3)*70)
   if m==0:tr.update(note=24+root+[0,12,7,12,19,0][i],tone=160+i*25,harm=100)
   if m==1:tr.update(note=48+root+i,decay=200+i*25)
   if m==2:tr.update(note=72,decay=120+i*25,level=270 if i==0 else 170)
  else:
   tr.update(level=[460,300,240,280,220,210][i],send=350,ratchet=1,hits=[2,3,1,3,2,1][i],rotate=[0,2,4,1,6,8][i])
   if fam!=5:tr.update(length=16,speed=2 if fam==2 else 1)
   if m in [3,4,6,7]:tr.update(attack=120 if fam==5 else 0,decay=420+(i%3)*110)
  # Rhythmic controls stay independently editable; the first voice anchors time.
  if i==0:tr.update(rotate=0,prob=1000)
  ix=b['tones'][i]
  if ix>=0 and not all(tr[k]==tones[ix][k] for k in tone_fields):b['tones'][i]=-1
empty=json.loads(json.dumps(biomes[0]));empty.update(name='Blank Biome',family=8,seed=0xD4EA125,macro=[500,0,500,0],feedback=0,grain_pitch=0,reverse=0)
for i,tr in enumerate(empty['tracks']):
 tr.update({k:tones[112][k] for k in tone_fields});tr.update(hits=0,length=16,rotate=0,prob=1000,send=350,attack=0,speed=1,level=550);empty['tones'][i]=-1
biomes.append(empty)
for i,b in enumerate(biomes):b['id']=i
# Additional 1.1 palettes; IDs 0-127 and all original Biomes are preserved.
bass_names=['Velvet Foundation','Deep Root','Round Step','Soft Rubber','Warm Undertow','Dub Anchor','Low Lantern','Sub Bloom','Wooden Pulse','Gentle Current','Round Motion','Night Floor','Felt Octave','Amber Bass','Slow Earth','Clear Foundation']
strike_names=['Felt Mallet','Moss Marimba','Wooden Bell','Clay Bowl','Amber Bar','Soft Glass','Muted Metal','Low Vessel','Gentle Pluck','Reed Pluck','Warm Tine','Quiet Wire','Sand Seeds','Pebble Rain','Dust Bells','Soft Chimes']
for model,names in [(8,bass_names),(9,strike_names)]:
 for j,name in enumerate(names):
  tones.append(dict(id=len(tones),name=name,model=model,tone=(80+j*25 if model==8 else 100+(j%8)*45),harm=(80+(j%5)*110 if model==8 else 80+(j%8)*95),decay=(80+(j%6)*55 if model==8 else 150+(j%6)*70),attack=0,shape=(100 if model==8 else 100 if j<8 else 500 if j<12 else 800),cutoff=(400+(j%4)*80 if model==8 else 450+(j%5)*60),resonance=0,filter=1,breathe=0))
# 1.1 Mellow collection: new IDs, existing 1.0 factory data unchanged.
for group,model in [('Mellow',10),('Velvet',8),('Hearth',4)]:
 names=['Candle','Linen','Dusk','Willow','Honey','Ember','Lagoon','Moss','Cloud','Dune','Cedar','Haze','Amber','Shelter','Wool','Moon']
 for j,name in enumerate(names):
  tones.append(dict(id=len(tones),name=group+' '+name,model=model,tone=60+(j%4)*65,harm=500 if model==4 else 80+(j%4)*70,decay=450+(j%4)*150,attack=[0,220,450,650][j%4],shape=120+(j%4)*180,cutoff=450+(j%4)*65,resonance=0,filter=1,breathe=100+(j%3)*100))
for j,name in enumerate(['Linen Evening','Honey Lagoon','Mellow Canopy','Candle Drift','Warm Shelter','Willow Haze','Amber Rest','Moonlit Wool']):
 b=json.loads(json.dumps(biomes[0]));b.update(id=len(biomes),name=name,tempo=56+j*3,root=[0,5,2,7,9,3,10,0][j],scale=4,macro=[500,0,480,240],feedback=180)
 for i,tr in enumerate(b['tracks']):
  ix=160+(j+i)%16 if i!=1 else 176+j
  tr.update({k:tones[ix][k] for k in tone_fields});tr.update(note=36+b['root']+[12,0,19,24,7,12][i],level=[460,270,220,180,200,160][i],length=16,hits=[1,2,1,1,2,1][i],rotate=[0,0,4,8,6,12][i],speed=4,prob=1000,send=250,attack=300+(i%3)*150,decay=850,resonance=0)
  b['tones'][i]=-1
 biomes.append(b)
# Satin Morph: original rounded wavetable family; preserve every earlier ID.
for j,name in enumerate(['Soft Sine','Wool Triangle','Hollow Reed','Rounded Saw','Dusky Organ','Linen Strings','Cedar Bass','Honey Pulse','Felt Keys','Slow Amber','Velour Fifth','Quiet Brass','Warm Current','Paper Flute','Deep Satin','Morning Ribbon']):
 tones.append(dict(id=len(tones),name=name,model=11,tone=700+(j%3)*100,harm=[0,333,665,1000][j%4],decay=500+(j%4)*130,attack=[0,250,450,600][j%4],shape=200+(j%4)*160,cutoff=650+(j%3)*65,resonance=0,filter=1,breathe=120))
for j,name in enumerate(['Satin River','Cedar Hearth','Velour Dawn','Soft Current']):
 b=json.loads(json.dumps(biomes[65+j]));b.update(id=len(biomes),name=name,tempo=64+j*6)
 for i,tr in enumerate(b['tracks']):
  if i!=1:
   ix=208+(j*3+i)%16;tr.update({k:tones[ix][k] for k in tone_fields});tr.update(attack=250+(i%3)*180,decay=850);b['tones'][i]=-1
 biomes.append(b)
# Monades: the FDDS Mondes, complete v10 patches after every Mutable Dreams ID.
from fdds_mondes import fdds,c_patch
biomes+=fdds(tones,fields,len(biomes))
catalog=dict(version='1.1',families=family_names+['Your own world','Flash Dog Duke Silver'],family_short=family_short+['BLANK','FDDS'],tone_fields=tone_fields,tones=tones,biomes=biomes)
(R/'presets/catalog.json').write_text(json.dumps(catalog,indent=2,ensure_ascii=False)+'\n')
lines=['/* Generated by tools/generate_presets.py; MIT. Flash-resident factory atlas. */','struct TonePreset { const char *name; int32_t '+','.join(tone_fields)+'; };','static const TonePreset tone_bank[MD_TONE_PRESETS]={']
for t in tones:lines.append('{'+json.dumps(t['name'])+','+','.join(str(t[k]) for k in tone_fields)+'},')
lines+=['};','struct BiomePreset { const char *name; unsigned family; md_patch patch; };','static const char *biome_families[]={'+','.join(json.dumps(x) for x in catalog['family_short'])+'};','static const BiomePreset biome_bank[MD_BIOMES]={']
for b in biomes:
 head=[0x4D445231,4,b['seed']];tail=[b[k] for k in ['tempo','locked','freeze','grain_size','grain_pos','feedback','quality']];gl=[b[k] for k in ['root','scale','transpose','swing','grain_pitch','reverse','duck_source','duck_amount','duck_release']]
 if b.get('v10'):lines.append('{'+json.dumps(b['name'])+','+str(b['family'])+',{'+c_patch(b,fields)+'}},');continue
 p=','.join(map(str,head))+',{'+','.join(map(str,b['macro']))+'},'+','.join(map(str,tail))+',{'+','.join('{'+','.join(str(t[k]) for k in fields)+'}' for t in b['tracks'])+'},'+','.join(map(str,gl))+','+str(b['id'])+',{'+','.join(map(str,b['tones']))+'},0'
 lines.append('{'+json.dumps(b['name'])+','+str(b['family'])+',{'+p+'}},')
lines+=['};'];(R/'app/dsp/preset_bank.inc').write_text('\n'.join(lines)+'\n')
print(f'{len(biomes)} Biomes / {len(tones)} tone presets generated')
