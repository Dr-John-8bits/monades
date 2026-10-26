#!/usr/bin/env python3
# SPDX-License-Identifier: MIT OR GPL-3.0-only
"""Mutable Dreams saved project/preferences, 16 Biomes and 64 sounds.
Export the live patch separately. Backup never implicitly saves it.
Restore requires stopped transport, internal clock and physical SEL on the FM-1.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'base/tools'))
import fm1_backup as B
KIND=B.Kind(magic=b'MDDB',version=1,objs=(0,2,*range(3,23)),
 names=lambda o:'Project' if o==0 else 'Preferences' if o==2 else f'Biome {o-2:02}' if o<19 else f'Tones {(o-19)*16+1}-{(o-18)*16}',
 identity='FM-1_710',name='Mutable Dreams',port='X0X FM-1')
KIND.saved_only=True
if __name__=='__main__':
 print('Saved objects only. Unsaved live edits, granular audio and calibration are excluded.')
 sys.exit(B.main(kind=KIND,description=__doc__))
