# SPDX-License-Identifier: GPL-3.0-only
"""The FDDS Mondes: complete v10 patches (LFOs, generators, form, gamakas, Monades effects), one per place or
character of the Flash Dog Duke Silver wiki. Read by tools/generate_presets.py; sounds start from the tone bank."""
import re
from pathlib import Path
R=Path(__file__).resolve().parents[1]
_T=(R/'app/dsp/scale_table.inc').read_text()
_twelve=re.findall(r'\{"([^"]+)",\d+,0x[0-9a-f]+,0x[0-9a-f]+\}',_T)
_micro=re.findall(r'\{"([^"]+)",\d+,\d+,[\d.]+f,',_T)
_first_micro=int(re.search(r'SCALE_TWELVE=(\d+)',_T).group(1))
SCALE={n:i for i,n in enumerate(_twelve)}|{n:_first_micro+i for i,n in enumerate(_micro)}
NOTE={'C':0,'C#':1,'D':2,'D#':3,'E':4,'F':5,'F#':6,'G':7,'G#':8,'A':9,'A#':10,'B':11}
LFO_RATE={'8 BARS':0,'4 BARS':1,'2 BARS':2,'1 BAR':3,'1/2':4,'1/4 .':5,'1/2 T':6,'1/4':7,'1/8 .':8,'1/4 T':9,'1/8':10,'1/8 T':11,'1/16':12,'1/16 T':13,'1/32':14,'FREE SLOW':15,'FREE':17}
LFO_SHAPE={'SINE':0,'TRIANGLE':1,'RAMP UP':2,'RAMP DOWN':3,'SQUARE':4,'S&H':5,'SMOOTH':6}
LFO_TARGET={'CUTOFF':0,'RESONANCE':1,'LEVEL':2,'PAN':3,'PITCH':4,'TONE':5,'HARMONICS':6,'SHAPE':7,'ECHO SEND':8,'ROOM SEND':9,'GRAIN SEND':10,'NOTES':17,'HITS':18}
ECHO={'OFF':0,'1/32':1,'1/16 T':2,'1/16':3,'1/8 T':4,'1/16 .':5,'1/8':6,'1/4 T':7,'1/8 .':8,'1/4':9}
GEN={'OFF':0,'RANDOM':1,'MARKOV':2,'MOTIF':3}
GAMAKA={'OFF':0,'KAN':1,'MEEND':2,'ANDOLAN':3,'MIX':4}

# Each Monde: tempo, root, scale, macros (density, motion, colour, space), effects, then six tracks
# (tone preset name, base note, hits/length, and anything that departs from it), LFOs, generators, gamakas.
MONDES=[
 dict(name='MACHIN',tempo=124,root='C',scale='DIM HALF-WH',macro=[620,220,450,120],
  echo='1/16',delay_fb=420,fx=460,room=300,
  tracks=[('Dub Heart',36,4,16,dict(level=650)),('Soft Metal',72,8,16,dict(level=240,filter=4,cutoff=700)),
   ('Dry Twig',50,2,16,dict(rotate=4,level=320)),('Copper Bell',48,5,12,dict(level=300,echo=300)),
   ('Round Step',36,3,16,dict(level=430)),('Glass Drop',60,3,7,dict(level=220,echo=400))],
  lfo={(3,0):('1/16','S&H','TONE',600)},gen={3:('RANDOM',1000,8,12),5:('RANDOM',1000,4,7)}),
 dict(name='GADIN',tempo=96,root='D',scale='SLENDRO 5-TET',macro=[540,300,420,200],
  fx=420,room=560,form=(800,2,2),
  tracks=[('Clay Drum',38,4,16,dict(level=560)),('Wood Bowl',48,5,16,dict(level=300,room=300)),
   ('Clay Bowl',55,3,8,dict(level=280)),('Night Voices',60,2,16,dict(level=230,attack=300,room=500)),
   ('Hollow Log',60,4,12,dict(level=270,room=300)),('Shaker Sand',72,7,16,dict(level=200))],
  gen={1:('RANDOM',500,8,7),4:('RANDOM',500,6,12)}),
 dict(name='MARTINI',tempo=92,root='G',scale='BEBOP DOM',swing=300,macro=[520,260,560,160],
  echo='1/8 .',delay_fb=300,fx=460,room=450,chorus=(400,600),
  tracks=[('Soft Knock',36,3,16,dict(level=520)),('Brush Skin',50,2,16,dict(rotate=4,level=300)),
   ('Warm Ride',72,6,16,dict(level=220)),('Velvet Foundation',43,3,16,dict(level=430)),
   ('Mellow Honey',55,4,16,dict(level=300,echo=350,room=300)),('Velour Fifth',55,2,16,dict(level=200,speed=4,attack=500,room=400))],
  phrase={4:[12,-1,16,-1,19,-1,22,-1,21,-1,19,-1,16,-1,14,-1]},gen={4:('MOTIF',400,8,6)}),
 dict(name='CHRISTELE',tempo=62,root='D',scale='BAGESHRI',macro=[540,320,520,260],
  echo='1/8 .',delay_fb=320,fx=560,room=750,diffusion=800,chorus=(350,500),vadi=5,samvadi=0,
  tracks=[('Night Voices',57,3,16,dict(level=330,speed=2,attack=400,echo=300,room=500)),('Deep Satin',50,2,16,dict(level=230,speed=4,attack=600,room=500)),
   ('Mellow Candle',62,2,12,dict(level=250,speed=2,room=400)),('Velvet Choir',55,2,16,dict(level=200,speed=4,attack=500,room=500)),
   ('Silk String',69,2,16,dict(level=200,speed=2,echo=400,room=400)),('Velvet Dusk',38,2,16,dict(level=330,speed=4))],
  phrase={0:[12,-1,15,-1,17,-1,-1,-1,15,-1,12,-1,10,-1,-1,-1]},gen={0:('MOTIF',500,8,3)},
  gamaka={0:('MEEND',800),2:('MEEND',600),4:('ANDOLAN',500)}),
 dict(name='SARL',tempo=108,root='F',scale='HARMONIC MAJ',macro=[600,180,480,120],
  fx=380,room=450,form=(1000,1,3),
  tracks=[('Stone Floor',36,4,16,dict(level=620,prob=1000)),('Wood Snare',50,2,16,dict(rotate=4,level=330,accent=600,prob=1000)),
   ('Slow Brass',53,3,16,dict(level=300,prob=1000,room=300)),('Amber Keys',65,4,16,dict(level=230,prob=1000)),
   ('Felt Octave',41,3,16,dict(level=430,prob=1000)),('Closed Dust',72,8,16,dict(level=220,prob=1000))],
  lfo={(5,0):('2 BARS','SINE','HITS',600)}),
 dict(name='CITRONS',tempo=132,root='E',scale='HIJAZ',macro=[640,280,500,120],
  echo='1/16 T',delay_fb=380,fx=400,room=300,
  tracks=[('Tight Seed',36,4,16,dict(level=600)),('Dry Twig',50,5,16,dict(level=300,ratchet=3,prob=800)),
   ('Glass Dust',74,11,16,dict(level=190,ratchet=2)),('Muted Pluck',52,6,16,dict(level=270,echo=300)),
   ('Dub Anchor',40,3,16,dict(level=420)),('Paper Snare',55,3,12,dict(level=240,ratchet=4,accent=800))],
  lfo={(1,0):('1 BAR','SQUARE','HITS',700),(5,0):('2 BARS','S&H','HITS',500)},gen={3:('RANDOM',300,8,8)}),
 dict(name='CARGO',tempo=54,root='D',scale='JUST MINOR',macro=[540,120,380,320],
  fx=600,room=850,send_hp=400,diffusion=900,
  tracks=[('Low Gong',36,2,16,dict(level=330,speed=4,room=500)),('Deep Spectrum',38,2,16,dict(level=260,speed=4,attack=800,filter=1,cutoff=600,room=500)),
   ('Low Partial',45,2,16,dict(level=220,speed=4,attack=600,room=500)),('Deep Vessel',50,2,12,dict(level=240,speed=4,room=600)),
   ('Night Voices',50,2,16,dict(level=200,speed=4,attack=700,room=600)),('Fine Rain',76,3,16,dict(level=130,speed=2,room=400))],
  lfo={(1,0):('FREE SLOW','SINE','CUTOFF',500),(4,0):(16,'SINE','LEVEL',400),(3,1):('FREE','TRIANGLE','PAN',600)}),
 dict(name='VEGA',tempo=88,root='E',scale='SPANISH 8',macro=[520,240,500,160],
  echo='1/8 .',delay_fb=450,fx=480,room=500,form=(700,2,1),
  tracks=[('Low Pebble',36,2,16,dict(level=560)),('Soft Steel',52,5,16,dict(level=300,echo=400)),
   ('Muted Rim',50,3,16,dict(rotate=4,level=280)),('Satin Bass',40,3,16,dict(level=420)),
   ('Hollow Flute',64,2,16,dict(level=230,speed=2,echo=500,room=400)),('Tiny Bell',79,1,16,dict(level=180))],
  gen={4:('RANDOM',600,8,12)}),
 dict(name='FENOUIL',tempo=84,root='G',scale='MAJOR PENT',macro=[500,260,520,220],
  fx=420,room=550,form=(600,2,1),
  tracks=[('Moss Kick',36,3,16,dict(level=540)),('Moss Marimba',60,5,16,dict(level=300,room=300)),
   ('Leaf Shake',72,6,16,dict(level=200)),('Mellow Moss',55,4,12,dict(level=260,speed=2,room=400)),
   ('Gentle Pluck',67,4,10,dict(level=230,room=300)),('Velvet Moss',43,3,16,dict(level=380))],
  phrase={1:[0,-1,4,7,-1,9,-1,7,12,-1,9,-1,7,4,-1,2],3:[7,-1,-1,4,-1,2,-1,-1,0,-1,2,-1,4,-1,-1,-1]},
  gen={1:('MARKOV',300,8,12),3:('MARKOV',400,8,12)}),
 dict(name='CLOUDEE',tempo=100,root='C',scale='24-TET',macro=[520,300,520,200],
  echo='1/16 T',delay_fb=500,fx=480,room=450,
  tracks=[('Soft Knock',36,3,16,dict(level=520)),('Sine Pearl',72,7,16,dict(level=230,echo=400)),
   ('Glass Drop',60,4,13,dict(level=250,echo=300)),('Cloud Reed',48,2,16,dict(level=230,speed=4,attack=500,room=400)),
   ('Mist Tick',74,5,16,dict(level=180)),('Slow Glass',84,2,9,dict(level=170,room=400))],
  lfo={(1,0):('1/8','S&H','PITCH',300),(2,0):('1 BAR','SMOOTH','NOTES',700)},gen={1:('RANDOM',200,8,24),2:('RANDOM',400,8,12)}),
 dict(name='HENDIATRIQUE',tempo=90,root='A',scale='IN',macro=[520,240,500,180],
  fx=420,room=500,
  tracks=[('Warm Pulse',36,2,6,dict(level=540)),('Muted Rim',50,2,9,dict(level=280)),
   ('Dry Leaf',74,2,3,dict(level=200)),('Clay Bell',57,5,12,dict(level=270,filter=1,cutoff=650,room=300)),
   ('Hollow Chime',69,4,9,dict(level=220,room=300)),('Amber Pad',45,2,12,dict(level=230,speed=4,attack=500,room=400))],
  lfo={(3,0):('1/4 T','TRIANGLE','CUTOFF',500),(4,0):('1/8 T','SINE','PAN',600)}),
 dict(name='ALCHIMISTE',tempo=70,root='C',scale='BOHLEN PIERCE',macro=[520,280,520,260],
  fx=540,room=700,chorus=(200,500),
  tracks=[('Ice Bell',60,3,16,dict(level=260,speed=2,room=400)),('Bronze Bloom',48,2,16,dict(level=240,speed=4,attack=400,room=400)),
   ('Copper Rain',72,5,13,dict(level=220,room=400)),('Glass Choir',60,2,16,dict(level=200,speed=4,attack=500,room=500)),
   ('Hollow Chime',84,2,11,dict(level=180,room=500)),('Low Gong',36,2,16,dict(level=300,speed=4,room=400))],
  gen={2:('RANDOM',400,8,19)}),
 dict(name='COGITO',tempo=66,root='C',scale='SHRUTI22',macro=[520,240,500,240],
  fx=520,room=700,diffusion=700,vadi=7,samvadi=0,
  tracks=[('Sine Lantern',48,2,16,dict(level=240,speed=4,attack=600,room=400)),('Velvet Choir',55,2,16,dict(level=210,speed=4,attack=500,room=500)),
   ('Silk String',60,3,16,dict(level=250,speed=2,room=400)),('Pure Harmonic',67,2,12,dict(level=220,speed=2,room=400)),
   ('Soft Tine',72,2,16,dict(level=220,speed=2,room=400)),('Velvet Foundation',36,2,16,dict(level=330,speed=4))],
  phrase={4:[0,-1,2,-1,4,-1,7,-1,-1,-1,4,-1,2,-1,-1,-1]},gen={4:('MARKOV',300,8,7)},
  gamaka={2:('ANDOLAN',500),3:('KAN',600)}),
 dict(name='JARDINIERS',tempo=98,root='F',scale='LYDIAN',macro=[520,240,560,200],
  echo='1/8',delay_fb=300,fx=440,room=500,chorus=(300,500),
  tracks=[('Soft Knock',36,3,16,dict(level=520)),('Fine Rain',74,5,16,dict(level=190)),
   ('Felt Mallet',60,6,16,dict(level=280,echo=300,room=300)),('Mellow Linen',53,2,16,dict(level=250,speed=2,room=300)),
   ('Morning Ribbon',65,2,16,dict(level=200,speed=4,attack=400,room=400)),('Wool Bass',41,2,16,dict(level=380))],
  phrase={2:[0,-1,4,-1,6,7,-1,-1,11,-1,7,-1,6,-1,4,-1]},gen={2:('MOTIF',500,8,5)}),
]

def fdds(tones,fields,first_id):
 """Catalog entries for generate_presets.py: the 1.1 fields plus an 'ext' block written as a complete v10 patch."""
 by_name={}
 for t in tones:by_name.setdefault(t['name'],t)
 out=[]
 for k,m in enumerate(MONDES):
  b=dict(name=m['name'],family=9,tempo=m['tempo'],root=NOTE[m['root']],scale=SCALE[m['scale']],seed=0xFD05000+k*7919,macro=m['macro'],
   locked=1,freeze=0,grain_size=650,grain_pos=300,feedback=250,quality=0,transpose=0,swing=m.get('swing',0),grain_pitch=0,reverse=0,
   duck_source=-1,duck_amount=0,duck_release=500,tracks=[],tones=[-1]*6,v10=True)
  ext=[];
  for i,(tone_name,note,hits,length,over) in enumerate(m['tracks']):
   t=by_name[tone_name];tr=dict.fromkeys(fields,0)
   tr.update({f:t[f] for f in ['model','tone','harm','decay','attack','shape','cutoff','resonance','filter','breathe']})
   tr.update(level=400,send=200,hits=hits,length=length,rotate=0,prob=900 if i else 1000,note=note,pan=[500,340,660,220,780,500][i],
    mute=0,speed=1,ratchet=1,accent=200)
   tr.update({f:v for f,v in over.items() if f in fields})
   if tr['hits']<=2 or tr['speed']>=4:tr['prob']=1000 # the seed is locked: Chance repeats each pass, a sparse track must not miss for good
   ext.append(dict(delay_send=over.get('echo',0),reverb_send=over.get('room',150),notes=m.get('phrase',{}).get(i,[0,-1,-1,-1,7,-1,-1,-1,0,-1,-1,-1,7,-1,-1,-1])))
   b['tracks'].append(tr)
  lfo=[[dict(rate=7,shape=0,target=0,depth=0) for s in range(2)] for i in range(6)]
  for (i,s),(rate,shape,target,depth) in m.get('lfo',{}).items():
   lfo[i][s]=dict(rate=LFO_RATE[rate] if isinstance(rate,str) else rate,shape=LFO_SHAPE[shape],target=LFO_TARGET[target],depth=depth)
  gen=[dict(source=0,dejavu=500,loop=8,spread=12) for i in range(6)]
  for i,(src,dv,loop,spread) in m.get('gen',{}).items():gen[i]=dict(source=GEN[src],dejavu=dv,loop=loop,spread=spread)
  gam=[(0,500)]*6
  for i,(style,amount) in m.get('gamaka',{}).items():gam[i]=(GAMAKA[style],amount)
  form=m.get('form',(0,1,2));chorus=m.get('chorus',(0,500))
  b['ext']=dict(tracks=ext,delay_ms=200,delay_fb=m.get('delay_fb',300),reverb_time=m.get('room',450),fx_level=m.get('fx',400),
   send_hp=m.get('send_hp',0),echo_tone=500,room_tone=500,lfo=lfo,echo_sync=ECHO[m.get('echo','OFF')],room_diffusion=m.get('diffusion',0),
   chorus=chorus[0],chorus_depth=chorus[1],gen=gen,form=form[0],form_bars=form[1],form_drops=form[2],
   gamaka=[g[0] for g in gam],gamaka_amount=[g[1] for g in gam],vadi=m.get('vadi',-1),samvadi=m.get('samvadi',-1),motion_every=0)
  b['id']=first_id+k;out.append(b)
 return out

def c_patch(b,fields):
 """The C initializer of a complete v10 md_patch, in md_patch's field order."""
 j=lambda xs:','.join(map(str,xs))
 e=b['ext']
 head=[0x4D445231,10,b['seed']];tail=[b[k] for k in ['tempo','locked','freeze','grain_size','grain_pos','feedback','quality']]
 gl=[b[k] for k in ['root','scale','transpose','swing','grain_pitch','reverse','duck_source','duck_amount','duck_release']]
 tracks='{'+','.join('{'+j(t[k] for k in fields)+'}' for t in b['tracks'])+'}'
 ext='{'+','.join('{'+j([x['delay_send'],x['reverb_send'],0,0,0,0,0,250,0,0])+',{'+j(x['notes'])+'}}' for x in e['tracks'])+'}'
 lfo='{'+','.join('{'+','.join('{'+j([l['rate'],l['shape'],l['target'],l['depth']])+'}' for l in row)+'}' for row in e['lfo'])+'}'
 gen='{'+','.join('{'+j([g['source'],g['dejavu'],g['loop'],g['spread']])+'}' for g in e['gen'])+'}'
 return (j(head)+',{'+j(b['macro'])+'},'+j(tail)+','+tracks+','+j(gl)+','+str(b['id'])+',{'+j(b['tones'])+'},0,'+ext+','
  +j([e['delay_ms'],e['delay_fb'],e['reverb_time'],e['fx_level'],e['send_hp'],e['echo_tone'],e['room_tone']])+','+lfo+','
  +j([e['echo_sync'],e['room_diffusion'],e['chorus'],e['chorus_depth']])+','+gen+','+j([e['form'],e['form_bars'],e['form_drops']])+','
  +'{'+j(e['gamaka'])+'},{'+j(e['gamaka_amount'])+'},'+j([e['vadi'],e['samvadi'],e['motion_every']]))

# The FDDS wiki page each Monde comes from (https://thomasclaissethcl-blip.github.io/FDDS/pages/<slug>.html).
WIKI={'MACHIN':'machins','GADIN':'jacques-homme-dore','MARTINI':'pornstar-martini','CHRISTELE':'christele-du-boudoir',
 'SARL':'napoleon-d-artagnan','CITRONS':'cuisine-des-enfers','CARGO':'cargo-synapse','VEGA':'vega','FENOUIL':'fenouil',
 'CLOUDEE':'intelligence-d-autoguidage-cloudee','HENDIATRIQUE':'hendiatrique','ALCHIMISTE':'pornstar-martini','COGITO':'le-cogito','JARDINIERS':'jardiniers'}

def site_block(first_id=77):
 """The manual's table of FDDS Mondes, read from MONDES: python3 tools/fdds_mondes.py writes it into site/nouveautes/index.html."""
 rows=[]
 for k,m in enumerate(MONDES):
  what=[]
  for (i,s),(rate,shape,target,depth) in sorted(m.get('lfo',{}).items()):
   shown={'S&H':'SAMPLE &amp; HOLD','SMOOTH':'SMOOTH RANDOM'}.get(shape,shape);rate=rate if isinstance(rate,str) and not rate.startswith('FREE') else 'FREE'
   what.append(f"LFO {'AB'[s]} piste {i+1}&nbsp;: {'NOTES IN SCALE' if target=='NOTES' else target}, {shown}, {rate}")
  for i,(src,dv,loop,spread) in sorted(m.get('gen',{}).items()):
   what.append(f"générateur {src} piste {i+1}"+(", déjà-vu 100&nbsp;%" if dv==1000 else ''))
  for i,(style,amount) in sorted(m.get('gamaka',{}).items()):what.append(f"gamaka {style} piste {i+1}")
  if m.get('vadi',-1)>=0:what.append('notes pivots')
  if m.get('form'):what.append(f"forme sur {[2,4,8,16][m['form'][1]]} mesures par section")
  if m.get('echo'):what.append(f"écho {m['echo']}")
  if m.get('chorus'):what.append('chorus')
  if m.get('send_hp'):what.append('passe-haut sur les envois')
  lengths=sorted({t[3] for t in m['tracks']})
  if len(lengths)>2:what.append('longueurs '+', '.join(map(str,lengths))+' pas')
  url='https://thomasclaissethcl-blip.github.io/FDDS/pages/'+WIKI[m['name']]+'.html'
  rows.append(f'      <tr><td>{first_id+k+1} <a href="{url}">{m["name"]}</a></td><td>{m["scale"]} · {m["root"]} · {m["tempo"]}&nbsp;BPM</td><td>{" · ".join(what)}</td></tr>\n')
 return ('<!-- fdds: generated by tools/fdds_mondes.py -->\n    <table class="manual">\n      <tr><th>Monde</th><th>Gamme · note · tempo</th><th>Ce qu\'il fait entendre</th></tr>\n'
  +''.join(rows)+'    </table>\n    <!-- /fdds -->')

if __name__=='__main__':
 for page in (R/'site/nouveautes/index.html',R/'site/manuel/index.html'):
  s=page.read_text();s,n=re.subn(r'<!-- fdds:.*?<!-- /fdds -->',lambda x:site_block(),s,flags=re.S);assert n==1,f'markers missing in {page}'
  page.write_text(s)
 print(len(MONDES),'FDDS Mondes in the manuals')
