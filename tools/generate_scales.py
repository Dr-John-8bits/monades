#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Writes app/dsp/scale_table.inc: python3 tools/generate_scales.py app/dsp/scale_table.inc
# Scala files (.scl) dropped in tools/scala/ are added to the SCALA family.
# Scale library for Monades: (name, family, ascent intervals, descent intervals or None). Indices 0-7 keep Mutable Dreams 1.1 order.
L=[
("CHROMATIC","BASIC",list(range(12)),None),
("MINOR","BASIC",[0,2,3,5,7,8,10],None),
("MAJOR","BASIC",[0,2,4,5,7,9,11],None),
("DORIAN","MODES",[0,2,3,5,7,9,10],None),
("MINOR PENT","PENTA BLUES",[0,3,5,7,10],None),
("MAJOR PENT","PENTA BLUES",[0,2,4,7,9],None),
("WHOLE TONE","SYMMETRIC",[0,2,4,6,8,10],None),
("PHRYGIAN","MODES",[0,1,3,5,7,8,10],None),
("LYDIAN","MODES",[0,2,4,6,7,9,11],None),
("MIXOLYDIAN","MODES",[0,2,4,5,7,9,10],None),
("LOCRIAN","MODES",[0,1,3,5,6,8,10],None),
("HARMONIC MIN","MINOR MAJOR",[0,2,3,5,7,8,11],None),
("MELODIC MIN","MINOR MAJOR",[0,2,3,5,7,9,11],[0,2,3,5,7,8,10]),
("JAZZ MINOR","MINOR MAJOR",[0,2,3,5,7,9,11],None),
("HARMONIC MAJ","MINOR MAJOR",[0,2,4,5,7,8,11],None),
("DOUBLE HARM","MINOR MAJOR",[0,1,4,5,7,8,11],None),
("HUNGARIAN MIN","MINOR MAJOR",[0,2,3,6,7,8,11],None),
("NEAPOL MINOR","MINOR MAJOR",[0,1,3,5,7,8,11],None),
("NEAPOL MAJOR","MINOR MAJOR",[0,1,3,5,7,9,11],None),
("PHRYG DOM","MINOR MAJOR",[0,1,4,5,7,8,10],None),
("LYDIAN DOM","MINOR MAJOR",[0,2,4,6,7,9,10],None),
("LYDIAN AUG","MINOR MAJOR",[0,2,4,6,8,9,11],None),
("MIXO B6","MINOR MAJOR",[0,2,4,5,7,8,10],None),
("HALF DIM","MINOR MAJOR",[0,2,3,5,6,8,10],None),
("ALTERED","MINOR MAJOR",[0,1,3,4,6,8,10],None),
("BLUES","PENTA BLUES",[0,3,5,6,7,10],None),
("MAJOR BLUES","PENTA BLUES",[0,2,3,4,7,9],None),
("EGYPTIAN","PENTA BLUES",[0,2,5,7,10],None),
("BEBOP DOM","BEBOP",[0,2,4,5,7,9,10,11],None),
("BEBOP MAJOR","BEBOP",[0,2,4,5,7,8,9,11],None),
("BEBOP DORIAN","BEBOP",[0,2,3,4,5,7,9,10],None),
("DIM HALF-WH","SYMMETRIC",[0,1,3,4,6,7,9,10],None),
("DIM WH-HALF","SYMMETRIC",[0,2,3,5,6,8,9,11],None),
("AUGMENTED","SYMMETRIC",[0,3,4,7,8,11],None),
("TRITONE","SYMMETRIC",[0,1,4,6,7,10],None),
("PROMETHEUS","SYMMETRIC",[0,2,4,6,9,10],None),
("BILAWAL","THAAT",[0,2,4,5,7,9,11],None),
("KALYAN","THAAT",[0,2,4,6,7,9,11],None),
("KHAMAJ THAAT","THAAT",[0,2,4,5,7,9,10],None),
("KAFI","THAAT",[0,2,3,5,7,9,10],None),
("ASAVARI","THAAT",[0,2,3,5,7,8,10],None),
("BHAIRAVI","THAAT",[0,1,3,5,7,8,10],None),
("BHAIRAV","THAAT",[0,1,4,5,7,8,11],None),
("MARWA THAAT","THAAT",[0,1,4,6,7,9,11],None),
("PURVI","THAAT",[0,1,4,6,7,8,11],None),
("TODI","THAAT",[0,1,3,6,7,8,11],None),
("YAMAN","RAGA",[0,2,4,6,7,9,11],None),
("BHUPALI","RAGA",[0,2,4,7,9],None),
("DURGA","RAGA",[0,2,5,7,9],None),
("MALKAUNS","RAGA",[0,3,5,8,10],None),
("HINDOL","RAGA",[0,4,6,9,11],None),
("MEGH","RAGA",[0,2,5,7,10],None),
("MARWA","RAGA",[0,1,4,6,9,11],None),
("DESH","RAGA",[0,2,5,7,11],[0,2,4,5,7,9,10]),
("KHAMAJ","RAGA",[0,4,5,7,9,11],[0,2,4,5,7,9,10]),
("JAUNPURI","RAGA",[0,2,5,7,8,10],[0,2,3,5,7,8,10]),
("BHIMPALASI","RAGA",[0,3,5,7,10],[0,2,3,5,7,9,10]),
("BAGESHRI","RAGA",[0,3,5,9,10],[0,2,3,5,9,10]),
("HIJAZ","MAQAM 12-TET",[0,1,4,5,7,8,10],None),
("HIJAZ KAR","MAQAM 12-TET",[0,1,4,5,7,8,11],None),
("NAHAWAND","MAQAM 12-TET",[0,2,3,5,7,8,11],[0,2,3,5,7,8,10]),
("KURD","MAQAM 12-TET",[0,1,3,5,7,8,10],None),
("NIKRIZ","MAQAM 12-TET",[0,2,3,6,7,9,10],None),
("NAWA ATHAR","MAQAM 12-TET",[0,2,3,6,7,8,11],None),
("HIRAJOSHI","JAPAN",[0,2,3,7,8],None),
("IN","JAPAN",[0,1,5,7,8],None),
("INSEN","JAPAN",[0,1,5,7,10],None),
("IWATO","JAPAN",[0,1,5,6,10],None),
("YO","JAPAN",[0,2,5,7,9],None),
("KUMOI","JAPAN",[0,2,3,7,9],None),
("RYUKYU","JAPAN",[0,4,5,7,11],None),
("PELOG ~","WORLD",[0,1,3,7,8],None),
("SLENDRO ~","WORLD",[0,2,5,7,9],None),
("PERSIAN","WORLD",[0,1,4,5,6,8,11],None),
("ORIENTAL","WORLD",[0,1,4,5,6,9,10],None),
("ENIGMATIC","WORLD",[0,1,4,6,8,10,11],None),
("SPANISH 8","WORLD",[0,1,3,4,5,6,8,10],None),
("MAJOR LOCRIAN","WORLD",[0,2,4,5,6,8,10],None),
("CUSTOM","CUSTOM",[0],None),
]
import math,os,sys
from fractions import Fraction
def cents(x):
    """Scala pitch: a ratio (3/2, 2) or cents (contains a dot)."""
    x=x.strip().split()[0]
    if "." in x:return float(x)
    return 1200*math.log2(float(Fraction(x)))
# Microtonal scales: (name, family, period in cents, degrees in cents above the root, the root itself excluded).
MICRO=[
("RAST","MAQAM 1/4",1200,[200,350,500,700,900,1050]),
("BAYATI","MAQAM 1/4",1200,[150,300,500,700,800,1000]),
("SABA","MAQAM 1/4",1200,[150,300,400,700,800,1000]),
("JUST MAJOR","MICROTONAL",1200,[cents(r) for r in "9/8 5/4 4/3 3/2 5/3 15/8".split()]),
("JUST MINOR","MICROTONAL",1200,[cents(r) for r in "9/8 6/5 4/3 3/2 8/5 9/5".split()]),
("PYTHAGOREAN","MICROTONAL",1200,[cents(r) for r in "9/8 81/64 4/3 3/2 27/16 243/128".split()]),
("24-TET","MICROTONAL",1200,[50*k for k in range(1,24)]),
("19-TET","MICROTONAL",1200,[1200*k/19 for k in range(1,19)]),
("SLENDRO 5-TET","MICROTONAL",1200,[240*k for k in range(1,5)]),
("THAI 7-TET","MICROTONAL",1200,[1200*k/7 for k in range(1,7)]),
]
def read_scl(path):
    """A Scala file: description, count, then one pitch per line; the last pitch is the period."""
    lines=[l for l in open(path,encoding="latin-1").read().splitlines() if not l.lstrip().startswith("!")]
    count=int(lines[1].split()[0]);pitches=[cents(l) for l in lines[2:2+count]]
    assert len(pitches)==count and count>=1
    name=os.path.splitext(os.path.basename(path))[0].upper().replace("_"," ")[:13]
    return (name,"SCALA",pitches[-1],pitches[:-1])
SCALA_DIR=os.path.join(os.path.dirname(os.path.abspath(__file__)),"scala")
if os.path.isdir(SCALA_DIR):
    MICRO+=[read_scl(os.path.join(SCALA_DIR,f)) for f in sorted(os.listdir(SCALA_DIR)) if f.lower().endswith(".scl")]
for n,f,per,deg in MICRO:
    assert len(n)<=13 and 0<per and len(deg)<=31 and all(0<d<per for d in deg) and deg==sorted(deg),n
FAM=["BASIC","MODES","MINOR MAJOR","PENTA BLUES","BEBOP","SYMMETRIC","THAAT","RAGA","MAQAM 12-TET","MAQAM 1/4","JAPAN","WORLD","MICROTONAL","SCALA","CUSTOM"]
def mask(iv):
    m=0
    for i in iv:m|=1<<i
    return m
assert all(len(n)<=13 for n,_,_,_ in L)
leg=[0xfff,0x5ad,0xab5,0x6ad,0x4a9,0x295,0x555,0x5ab]
for i,v in enumerate(leg):assert mask(L[i][2])==v and L[i][3] is None,(i,L[i])
ALL=[(n,f) for n,f,_,_ in L]+[(n,f) for n,f,_,_ in MICRO]
order=[i for f in FAM for i,x in enumerate(ALL) if x[1]==f]
assert sorted(order)==list(range(len(ALL)))
out=["/* Generated for Monades (scale table): name, family, ascent mask, descent mask (bit k = k semitones above the root).",
" * Indices 0-7 keep the Mutable Dreams 1.1 order so saved patches play the same. CUSTOM's mask comes from md_set_custom_scale(). */",
"struct ScaleDef { const char *name; uint8_t family; uint16_t up,down; };",
"static const char *const scale_families[]={%s};"%",".join('"%s"'%f for f in FAM),
"static const ScaleDef scale_defs[]={"]
for n,f,u,d in L:out.append(' {"%s",%d,0x%03x,0x%03x},'%(n,FAM.index(f),mask(u),mask(d if d else u)))
out.append("};")
out.append("static const uint8_t scale_order[]={%s}; /* browsing order, grouped by family */"%",".join(map(str,order)))
out.append("/* Microtonal scales (index SCALE_TWELVE + k): period and degrees in cents above the root. */")
out.append("struct MicroDef { const char *name; uint8_t family,count; float period; float degree[31]; };")
out.append("static const MicroDef micro_defs[]={")
for n,f,per,deg in MICRO:out.append(' {"%s",%d,%d,%.4ff,{%s}},'%(n,FAM.index(f),len(deg),per,",".join("%.4ff"%d for d in deg)))
out.append("};")
out.append("enum { SCALE_TWELVE=%d, SCALE_COUNT=%d, SCALE_CUSTOM=%d };"%(len(L),len(ALL),len(L)-1))
open(__import__('sys').argv[1],"w").write("\n".join(out)+"\n")
print(len(ALL),"scales,",len(MICRO),"microtonal")
