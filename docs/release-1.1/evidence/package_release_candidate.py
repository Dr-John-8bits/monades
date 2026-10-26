"""Prepare a private, versioned release candidate. Never publish or access MIDI."""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,sys,zipfile
root=Path.cwd();q=root/'work/release-qualification';src=root/'work/mutable-dreams-release-1.1'
out=root/'outputs/Mutable-Dreams-1.1-Release-Candidate';docs=src/'docs/release-1.1'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
result=json.loads((q/'release-device.json').read_text());assert result['pass_'] and result['device_free']
fw=src/'app/build/mutable-dreams-1.1.0.fwsc';h=sha(fw);assert result['firmware_sha256']==h
comparison=json.loads((q/'release-source-comparison.json').read_text());assert comparison['dsp_and_platform_unchanged']
direct=result['direct_1_0_upgrade'];assert direct['all_original_fields_exact'] and direct['all_22_original_objects_restored']
report=f'''MUTABLE DREAMS 1.1 — RELEASE BUILD QUALIFICATION
25 October 2026. Qualification record; publication receipts are tracked separately.

Firmware: Mutable-Dreams-1.1.0.fwsc
SHA256: {h}
USB identity: FM-1_710010100. MIDI port: X0X FM-1.
Size: {fw.stat().st_size} bytes. Application: 308972 bytes.
Static RAM 71768 / 98304; pool 334800 / 344064 (9264 bytes spare).
Audio callback 256 frames / 5804 us; DSP processing tiles 64 frames.

WHAT CHANGED AFTER THE LONG QUALIFICATION
The System page now says MUTABLE DREAMS 1.1. APP_VERSION is 1.1.0 instead of
1.1.0-DEV. Of 368 application/platform source files compared, only that one
display string differs. All twelve separately compiled DSP object files and
the 6980-byte update loader are byte-identical to the qualified 427a build.
Web/recovery copy, package names and export metadata carry the release version.
No DSP, preset parameters, MIDI protocol, storage layout or OTA code changed.

EXACT FINAL FILE — DIRECT 1.0 UPDATE
The unchanged published 1.0 (SHA256
75008881683cbeb3515e24cfd7830136b90eb293f0752dc66b203b7f2ee7cd9f)
was installed on the actual FM-1. A known custom Biome was saved/read back in
legacy v4 format, and this final 1.1 file was installed directly, without V15.
First boot migrated to v6: all original parameter bytes matched exactly.
The other 21 saved objects survived. The deliberately changed startup project
was then restored via normal import + SAVE; all 22 original objects and the
separate unsaved user live patch matched the fresh private backup exactly.
First boot reported zero timing events; the stopped restoration of the saved
project added one. That counter remained unchanged during the playback windows
below. This is recorded separately from audio-compute overloads and was not reset
or hidden. Flash writes can delay the audio interrupt even while stopped.

EXACT FINAL FILE — PLAYBACK / EDITOR / BANKS
'''
for row in result['rows']:
    assert not row['faults']
    report+=f"{row['name']}: {row['seconds']:.1f} s, peak callback {row['peak_callback_percent']:.2f}%, PASS.\n"
report+='''
Both dense six-track tests moved through all nineteen pages, varied Grain Size
through 19/20/21 and larger values, and read the complete bank backup during
playback. All six tracks triggered. USB audio was finite, audible and complete.
Zero new xruns, overloads, nonfinite samples, dropped commands, automatic quality
reductions or host capture overruns in these measured playback windows.
An existing named User Biome was loaded and re-saved with precisely its original
payload/name. Readback passed. The final complete backup still matches all 22
objects. The original unsaved live patch was restored; HOME, stopped, internal
clock, physical keys and master released. No private backup is distributed.

The initial bench process could not open its CoreAudio input after the two
firmware re-enumerations (PortAudio -9986 / AUHAL invalid property). No playback
test had begun. Reopening the bench in a fresh process, without reflashing or
changing firmware, restored capture. The first failure log is retained; the
successful playback measurements above come from the fresh process.

BUILD / OFFLINE CHECKS
Device compiler and memory/register/ROM checks passed without warnings/errors.
Rebuilt WASM: nineteen pages, legacy import, malformed-file rejection, live JSON
roundtrip, User Biome/Sound bank save-load/overwrite guard/restart, all 64 added
warm tones. Sanitized native editor/actual storage-map tests passed, including
130 simulated interrupted-write positions. These are simulations, not 130
physical power cuts. Packaged HTML/JS syntax, the Density focus regression,
binary packing, CRC/corrupt file handling, command serialization and transfer
locking passed. Installer tests check the embedded exact firmware/hash,
permission refusal and resumed-update identity checks with a fake updater.

INHERITED EVIDENCE — EXPLICIT SCOPE
Functional predecessor 427a34eedd2cc625b7833f3f953436bc1db1845f94cde695d8eed47332d09534
passed an actual Stock V15 roundtrip and physically confirmed full-bank restore,
physical cold start, two five-minute dense six-track tests and 64800 USB MIDI
messages. The eight earlier untimestamped events were investigated: stopped
flash maintenance reproduced timing events, while cold-start playback tests
remained at zero. Saving/restoring is a stopped/internal-clock operation, not
seamless audio-time flash writing. Detailed historical reports remain in source.

The V15 roundtrip was not repeated on this label-only build. The update loader,
platform/storage sources and all DSP objects are identical; direct upgrade and
the playback/storage checks above were repeated on the exact final file.
Do not relabel the 427a V15 measurements as a new exact-file V15 test.

VISIBLE BROWSER CHECKS
The owner confirmed browser audio/controls/User Biome save-load, live connection,
bidirectional Density, JSON/.mddb downloads, disconnect, and successful browser
reinstallation of 427a. Independent readback found all saved data unchanged.
Final package only changes release copy/metadata and the app's version text;
editor/installer protocol logic is unchanged. New package JS/embedding checks
passed. No automated rendered-GUI test is claimed: local URL access was denied
and was not bypassed. On 25 October the owner also confirmed completion of the
remaining browser Import and Restore checks. This is an owner report; no new
automated GUI recording or independent per-dialog readback is claimed.
TRS MIDI input is implemented and retained in this release at the owner's
request. The physical TRS connection remains untested. USB success does not
certify TRS wiring. End-to-end latency/all possible patch combinations unmeasured.

RELEASE / MEDIA
Published 1.0 and the developer handoff remain unchanged.
The completed English 1.1 product film uses calm male narration, seven real-device
audio recordings and clearly labeled native Mutable Dreams 1.1
firmware views rendered from source. No stock-firmware photograph is shown. It is not a camera recording of device operation. The
film is embedded in the staged release page. A portrait Instagram reel is
packaged separately. Published URLs are recorded separately after deployment.
'''
audio=root/'outputs/Mutable-Dreams-1.1-Release-Media/Audio-FM1/Capture-Report.json'
audio_rows=json.loads(audio.read_text()) if audio.exists() else []
if len(audio_rows)==7:
    assert all(x['firmware_sha256']==h and not x['host_flags'] for x in audio_rows)
    restored=json.loads((q/'release-audio/capture-restored.json').read_text())
    assert restored['latest_post_editor_live_restored'] and restored['clips_completed']==7
    report+=f'\nFINAL-BUILD MUSIC CAPTURES\nSeven new real-FM-1 recordings ({sum(x["seconds"] for x in audio_rows):.2f} seconds)\nwere captured from this exact release file, including warm ambient, bass/dub,\npercussion and Grain Size 20/21/50/100. No new device faults or host capture\noverruns occurred. Raw 44.1-kHz stereo WAVs retain their original level; the\n24-bit container does not imply 24-bit source precision. The user live patch\nwas restored and no saved bank was written. Audio production files are separate\nfrom firmware downloads; the completed product film is supplied separately.\n'
(docs/'QUALIFICATION.txt').write_text(report);(out/'DEVICE-TEST-REPORT.txt').write_text(report)
for n in ['START-HERE.txt','Recovery-1.1.txt']:shutil.copy2(docs/n,out/n)
shutil.copy2(fw,out/'Mutable-Dreams-1.1.0.fwsc')
for n in ['LICENSE','LICENSE-MIT.txt','LICENSING-GPL.txt']:shutil.copy2(src/n,out/n)
ev=docs/'evidence';ev.mkdir(exist_ok=True)
for n in ['release-device-build.log','release-web-build.log','release-native-build.log','release-wasm-tests.log','release-transfer-tests.log','release-range-tests.log','release-editor-tests.log','release-source-comparison.json','release-device.json','release-device-tests.log','release-audio-reconnect-first.log','test_release_device.py','package_release_candidate.py']:
    shutil.copy2(q/n,ev/n)
for n in ['direct-upgrade.json','direct_upgrade.py','restore_after_direct.py']:
    shutil.copy2(q/'release-direct'/n,ev/('final-'+n))
if len(audio_rows)==7:
    shutil.copy2(audio,ev/'final-audio-report.json')
    for n in ['capture.py','capture.log','capture-restored.json','verify_finish.py','final-device-state.json','final-readback.log']:
        shutil.copy2(q/'release-audio'/n,ev/('final-audio-'+n))
subprocess.run([sys.executable,str(src/'tools/package_release_installer.py'),str(fw),str(out/'Mutable-Dreams-1.1-Install.html'),str(q/'release-device.json')],check=True)
node='/Users/christophlink/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
with (q/'release-installer-tests.log').open('w') as log:
    subprocess.run([node,str(src/'tests/development_installer.mjs'),str(out/'Mutable-Dreams-1.1-Install.html'),str(fw)],stdout=log,stderr=subprocess.STDOUT,check=True)
shutil.copy2(q/'release-installer-tests.log',ev/'release-installer-tests.log')
# Preserve original component notices with the release and the complete source.
notices=out/'Notices';notices.mkdir(exist_ok=True)
for n in ['LICENSE','CREDITS.md','LICENSING.md']:
    shutil.copy2(src/'base'/n,notices/('fm1-base-'+n))
for p in (src/'app/third_party').rglob('*'):
    if p.is_file() and ('license' in p.name.lower() or 'copying' in p.name.lower()):
        dest=notices/p.relative_to(src/'app');dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest)
for p in (src/'base/assets/fonts').rglob('*'):
    if p.is_file() and ('ofl' in p.name.lower() or 'license' in p.name.lower()):
        dest=notices/p.relative_to(src);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest)
def source_files():
    for p in sorted(src.rglob('*')):
        rel=p.relative_to(src)
        if p.is_file() and not any(x in rel.parts for x in ['build','__pycache__','.git']) and p.name!='.DS_Store':
            assert p.suffix not in ['.raw','.mddb','.fwsc'],rel
            yield p,rel
(src/'SOURCE-SHA256.txt').write_text(''.join(f'{sha(p)}  {rel.as_posix()}\n' for p,rel in source_files() if p.name!='SOURCE-SHA256.txt'))
archive=out/'Mutable-Dreams-1.1-Source.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,6) as z:
    for p,rel in source_files():z.write(p,'mutable-dreams-1.1/'+rel.as_posix())
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    for line in (src/'SOURCE-SHA256.txt').read_text().splitlines():
        expected,n=line.split('  ',1);assert hashlib.sha256(z.read('mutable-dreams-1.1/'+n)).hexdigest()==expected
for n in ['Mutable-Dreams-1.1.html','Mutable-Dreams-1.1-Install.html']:
    for url in re.findall(r'(?:href|src)="([^\"]+)"',(out/n).read_text()):
        if url.startswith(('http:','https:','data:','#','mailto:')):continue
        assert (out/url.split('#')[0].split('?')[0]).exists(),(n,url)
protected={'outputs/Mutable-Dreams-1.0.fwsc':'75008881683cbeb3515e24cfd7830136b90eb293f0752dc66b203b7f2ee7cd9f','outputs/Mutable-Dreams-Developer-Handoff-2026-10-24.zip':'630e34663b3935db22a353cf781ab591ca438a1b82cfa1854e526bc361944c55'}
for n,want in protected.items():assert sha(root/n)==want
(out/'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.relative_to(out).as_posix()}\n' for p in sorted(out.rglob('*')) if p.is_file() and p.name not in ['SHA256SUMS.txt','.DS_Store']))
bundle=out.parent/(out.name+'.zip')
with zipfile.ZipFile(bundle,'w',zipfile.ZIP_DEFLATED,6) as z:
    for p in sorted(out.rglob('*')):
        if p.is_file():z.write(p,out.name+'/'+p.relative_to(out).as_posix())
with zipfile.ZipFile(bundle) as z:assert z.testzip() is None
status=dict(status='Release package prepared; publication tracked separately in work/publish-1.1/RESUME.json.',firmware_sha256=h,source_sha256=sha(archive),bundle_sha256=sha(bundle),source=str(src),package=str(out),private_backup='outputs/Mutable-Dreams-Private-Backup-2026-10-25-Release-Label',running_hardware_process=None,last_device_state='Release-labeled 1.1, all 22 saved objects exact, original live restored. Device reserved for owner listening.',published_1_0_unchanged=True,remaining=['Owner listening on release-labeled 1.1','Physical TRS remains untested; integrated at owner request','Website/GitHub/YouTube publication and Sven email tracked in work/publish-1.1/RESUME.json'])
(q/'RESUME.json').write_text(json.dumps(status,indent=2)+'\n')
p=root/'work/device-mellow/RESUME.json';j=json.loads(p.read_text());j.update(status=status['status'],firmware_sha256=h,source_zip=str(archive),source_zip_sha256=sha(archive),preview=str(out),backup=status['private_backup'],development_only=False,private_release_candidate=True,published=False,remaining=status['remaining'],followup_resume=str(q/'RESUME.json'),current_release=status);p.write_text(json.dumps(j,indent=2)+'\n')
with (root/'PROJEKTSTAND.txt').open('a') as f:
    f.write(f'\n25 October 2026 — Release-labeled 1.1 compiled and installed. SHA256 {h}. Exact final published-1.0 -> 1.1 migration, dense six-track/effects/editor/backup playback, warm Biomes, identical-content User Biome save/load and complete private data restoration passed. Separate private release package: {out}. Source/DSP/platform comparison and qualification scope included. V15 roundtrip evidence remains 427a with identical loader/platform/DSP; not retested on final hash. No publication. Resume work/release-qualification/RESUME.json.\n')
print(json.dumps(status,indent=2))
