from pathlib import Path
import subprocess,tomllib
r=Path('work/mutable-dreams-next').resolve();a=r/'app';b=r/'base';out=Path('work/device-mellow/plectrum-check').resolve();out.mkdir(exist_ok=True)
conf=tomllib.loads((a/'fm1.toml').read_text())
flags=['-O3','-ffp-contract=off','-DPLAT_HOST','-DTEST','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-isysroot','/Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk','-std=c++11',*[f'-I{x}' for x in [a,a/'dsp',a/'compat',a/'third_party',a/'build/gen',b/'host',b/'device',b/'include',b/'lib']]]
source=(a/'dsp/dreams.cpp').read_text();fixed='.05f+.15f*(t.shape-334)/332.0f'
assert fixed in source
old=source.replace('t.shape>=334&&t.shape<667?'+fixed+':.1f+.8f*unit(t.shape)', '.1f+.8f*unit(t.shape)')
assert old!=source
(out/'dreams-old.cpp').write_text(old)
text=(r/'tests/next_features.cpp').read_text();head=text.split('int main(){')[0];body=text.split(' // Rapid plucks')[1].split(' puts("ALL NEXT')[0]
# Isolate the true regression, with the same deterministic 5.8-second render.
(out/'test.cpp').write_text(head+'int main(){ md_patch p;\n // Rapid plucks'+body+'}\n')
objs=[a/'build/san'/(x.replace('/','_').replace('.','_')+'.o') for x in conf['o2'] if x!='dsp/dreams.cpp']
subprocess.run(['clang++',*flags,'-fno-exceptions','-fno-rtti','-c',str(out/'dreams-old.cpp'),'-o',str(out/'old.o')],check=True)
for label,obj in [('old',out/'old.o'),('fixed',a/'build/san/dsp_dreams_cpp.o')]:
 exe=out/label
 subprocess.run(['clang++',*[f for f in flags if f!=f'-I{a / "compat"}'],str(out/'test.cpp'),*map(str,objs),str(obj),'-o',str(exe)],check=True)
 p=subprocess.run([str(exe)],capture_output=True,text=True);print(label,'exit',p.returncode,p.stdout,p.stderr,flush=True)

 assert (p.returncode!=0 if label=='old' else p.returncode==0),label+' regression outcome'
