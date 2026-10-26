#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Writes app/dsp/scale_table.inc: python3 tools/generate_scales.py app/dsp/scale_table.inc
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
FAM=["BASIC","MODES","MINOR MAJOR","PENTA BLUES","BEBOP","SYMMETRIC","THAAT","RAGA","MAQAM 12-TET","JAPAN","WORLD","CUSTOM"]
def mask(iv):
    m=0
    for i in iv:m|=1<<i
    return m
assert all(len(n)<=13 for n,_,_,_ in L)
leg=[0xfff,0x5ad,0xab5,0x6ad,0x4a9,0x295,0x555,0x5ab]
for i,v in enumerate(leg):assert mask(L[i][2])==v and L[i][3] is None,(i,L[i])
order=[i for f in FAM for i,x in enumerate(L) if x[1]==f]
assert sorted(order)==list(range(len(L)))
out=["/* Generated for Monades (scale table): name, family, ascent mask, descent mask (bit k = k semitones above the root).",
" * Indices 0-7 keep the Mutable Dreams 1.1 order so saved patches play the same. CUSTOM's mask comes from md_set_custom_scale(). */",
"struct ScaleDef { const char *name; uint8_t family; uint16_t up,down; };",
"static const char *const scale_families[]={%s};"%",".join('"%s"'%f for f in FAM),
"static const ScaleDef scale_defs[]={"]
for n,f,u,d in L:out.append(' {"%s",%d,0x%03x,0x%03x},'%(n,FAM.index(f),mask(u),mask(d if d else u)))
out.append("};")
out.append("static const uint8_t scale_order[]={%s}; /* browsing order, grouped by family */"%",".join(map(str,order)))
out.append("enum { SCALE_COUNT=%d, SCALE_CUSTOM=%d };"%(len(L),len(L)-1))
open(__import__('sys').argv[1],"w").write("\n".join(out)+"\n")
print(len(L),"scales")
