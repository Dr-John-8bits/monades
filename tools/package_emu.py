#!/usr/bin/env python3
"""The drawn-panel simulator: fm1-base's emulator page (or app/web/emu/index.html) around the Monades WASM.
Run after `python tools/build_local.py web`; serve the output folder over http (an AudioWorklet needs it)."""
from pathlib import Path
import shutil,sys,tomllib
R=Path(__file__).resolve().parents[1];A=R/'app';out=Path(sys.argv[1] if len(sys.argv)>1 else A/'build/emu');out.mkdir(parents=True,exist_ok=True)
conf=tomllib.loads((A/'fm1.toml').read_text())
page=(A/'web/emu/index.html') if (A/'web/emu/index.html').exists() else R/'base/web/emu/index.html'
(out/'index.html').write_text(page.read_text().replace('{{NAME}}',conf.get('name','Monades')).replace('{{ID}}',conf.get('name','Monades')))
(out/'worklet.js').write_text((R/'web/worklet.js').read_text().replace('import "./app_worklet.js";\n','')) # the page loads app_worklet.js itself
shutil.copy(R/'web/app_worklet.js',out/'app_worklet.js')
shutil.copy(A/'build/wasm/dreams.wasm',out/'app.wasm')
if (R/'site/ecrans/live.png').exists():shutil.copy(R/'site/ecrans/live.png',out/'still.png') # the screen before switching on
print(out)
