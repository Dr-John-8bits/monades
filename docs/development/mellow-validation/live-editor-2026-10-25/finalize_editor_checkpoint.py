"""Seal only the measured 427a candidate; never publish or access the device."""
from pathlib import Path
import hashlib,json,shutil,zipfile,sys,subprocess
root=Path.cwd();r=root/'work/device-mellow';src=root/'work/mutable-dreams-next'
out=root/'outputs/Mutable-Dreams-1.1-Mellow-Preview';docs=src/'docs/development'
private=root/'outputs/Mutable-Dreams-Private-Backup-2026-10-24'
HF='427a34eedd2cc625b7833f3f953436bc1db1845f94cde695d8eed47332d09534'
fw=src/'app/build/mutable-dreams-1.1.0-dev.fwsc'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(fw)==HF
rows=json.loads((r/'editor-load-stress.json').read_text())
assert len(rows)==5 and all(not x['issues'] and x['firmware_sha256']==HF and x['cpu_peak_percent']<85 for x in rows)
state=json.loads((r/'final-editor-device-state.json').read_text())
assert state['firmware_sha256']==HF and all(state[k] for k in ['original_live_identical','all_22_saved_objects_identical','physical_master_released','internal_clock','stopped','home'])
roundtrip=json.loads((r/'qualified-editor-roundtrip.json').read_text())
assert roundtrip['firmware_sha256']==HF and len(roundtrip['steps'])==3 and all(x['pass_'] for x in roundtrip['steps'])
for name,token in [('qualified-restore.log','PASS real device restore'),('qualified-browser-protocol.log','22-bank backup twice identical'),('editor-transfer-final-tests.log','backup test passed'),('editor-dsp-tests.log','ALL DSP TESTS PASSED'),('editor-wasm-final-tests.log','"result": "PASS"'),('editor-browser-protocol-final-test.log','PASS UI transfer lock')]:
 assert token in (r/name).read_text(),name
protected=[('outputs/Mutable-Dreams-1.0.fwsc','75008881683cbeb3515e24cfd7830136b90eb293f0752dc66b203b7f2ee7cd9f'),('outputs/Mutable-Dreams-Developer-Handoff-2026-10-24.zip','630e34663b3935db22a353cf781ab591ca438a1b82cfa1854e526bc361944c55')]
for p,h in protected:assert sha(root/p)==h,p
(r/'editor-protected-artifacts.json').write_text(json.dumps([dict(file=p,sha256=h,unchanged=True) for p,h in protected],indent=2))
roundtrip.update(status='Exact final candidate -> Stock V15 -> same candidate, physically confirmed restore and full readback passed.',restore_physically_confirmed=True,all_22_saved_objects_identical=True,original_live_restored=True)
(r/'qualified-editor-roundtrip.json').write_text(json.dumps(roundtrip,indent=2))
remaining=['Owner listening on headphones and speakers, especially warm Biomes and Granular Size above 20','Interactive browser GUI/Web MIDI permission and device workflow test on a supported secure page','Extended cold-start/power sessions, physical TRS cabling and longer MIDI/jitter qualification; localize 8 timing events already present before the measured playback windows','Website/video/YouTube/Instagram/Sven preparation and publication after final release approval']
qualification=dict(date='2026-10-25',pass_=True,scope='Automated software and specified real-device tests; not public-release approval',firmware_sha256=HF,stock_sha256=roundtrip['stock_sha256'],device='FM-1_710010100',exact_stock_roundtrip=True,physical_restore=True,all_22_saved_objects_identical=True,original_live_restored=True,stock_erases_user_banks_and_preferences=True,stress=[{k:x[k] for k in ['name','seconds','cpu_peak_percent','rms','issues','backup_while_playing']} for x in rows],peak_callback_percent=max(x['cpu_peak_percent'] for x in rows),remaining=remaining,public_release_approved=False)
(r/'editor-qualification.json').write_text(json.dumps(qualification,indent=2))
history=docs/'mellow-validation/live-editor-2026-10-25';history.mkdir(parents=True,exist_ok=True)
for name in ['DEVICE-TEST-REPORT.txt','REQUIREMENTS-AUDIT.txt','BANK-RESTORE-INTEGRATION.txt']:
 old=history/('Before-Editor-'+name)
 if not old.exists():shutil.copy2(docs/name,old)
files=['editor-qualification.json','qualified-editor-roundtrip.json','qualified-editor-roundtrip.log','qualified-restore.log','qualified-browser-protocol.log','editor-load-stress.py','editor-load-stress.json','editor-load-stress.log','editor-device-qualified-build.log','editor-native-final-build.log','editor-transfer-final-tests.log','editor-dsp-tests.log','editor-web-final-build.log','editor-wasm-final-tests.log','editor-browser-protocol-final-test.log','editor-protected-artifacts.json','final-editor-device-state.json','stock-return-bank-diff.json']
for name in files:shutil.copy2(r/name,history/name)
(history/'PROVENANCE.txt').write_text(f'''Final installed and roundtrip-qualified firmware: {HF}
qualified-* and editor-load-stress identify this exact binary.
The earlier full restore test (including a temporary slot) used 5820845f;
the final physical restore and all 22 objects were reverified on 427a.
The legacy saved-project validator changed after 582; it now decodes old project
versions without touching live audio. Native legacy/DSP/WASM tests were rerun.
This checkpoint adds no synthesis algorithms and changes no DSP render hot path.
Live browser JS was exercised over real MIDI through a Node/Python transport.
That proves protocol behavior, not a browser permission dialog, rendering or GUI.
Source bundles exclude private .mddb/raw exports and manufacturer firmware.
Earlier reports and audio retain their actual firmware hashes.
''')
report=f'''MUTABLE DREAMS 1.1 — LIVE EDITOR / RECOVERY DEVICE QUALIFICATION
25 October 2026. Private development candidate; published 1.0 unchanged.

CURRENT INSTALLED FWSC SHA256: {HF}
Identity FM-1_710010100. Package 610136 bytes. Patch v6, 1292 bytes.
Application 308984 bytes; app-slot headroom 272580 bytes.
RAM .data+.bss 71768 / 98304 bytes; pool 334800 / 344064 (9264 spare).
One audio core. 256-frame callback / 5804 microseconds; DSP tiles 64 frames.
CPU percentages below measure audio callback time, not host or total UI usage.
End-to-end latency has not been measured.

COMPLETED ON THE ACTUAL FM-1
The exact package above was installed, changed to original Stock V15, and
reinstalled from V15; each restarted identity was checked by the installer.
V15 identity FM-1_015, package SHA256:
{roundtrip['stock_sha256']}
User physically confirmed the final restore with SEL. All 22 saved objects
were read back twice through the shipped JavaScript client and compared with
the complete pre-update backup. A final independent Python backup also matched.
This includes the saved project, preferences, 16 Biomes and 64 sounds.
IMPORTANT: Stock V15 erased the User Banks and preferences. Their recovery
required the .mddb backup after reinstalling 1.1. Do not promise preservation
through V15. Calibration is excluded from transfers.

The shipped browser protocol client passed real MIDI handshake/capabilities,
all 19 page states, complete live-patch upload/readback, and full bank restore.
Its Node/Python transport test is NOT an interactive browser GUI test.
Earlier integration checks additionally verified overwrite/playing guards and
automatic release of remote keys/master after a lost control lease.

EXACT FINAL BINARY — AUDIO, EDITING AND BACKUP TOGETHER
Six Harmonic voices, then Elements/Rings/VA/Satin/Bass/Choir: 220 BPM, dense
repeats, LP24, granular sends/feedback, shared delay/room and moving parameters.
Both stress cases visited all 19 pages and swept Grain Size through 19/20/21
and larger values, while transferring a byte-checked full backup during playback.
No new xruns, overloads, nonfinite samples, dropped audio or quality reductions;
every stress track triggered and captured USB audio stayed finite and audible.
Checks require at least 15% measured callback reserve for these finite cases.
The lifetime xrun counter was already 8 before the first measured case and
remained 8 through every case and final live-patch restoration. The preceding
boot/restore/editor sequence was not instrumented at each boundary, so these
earlier timing events cannot be attributed conclusively. They are not concealed
or relabelled as zero: localize them during extended startup/restore qualification.

'''
for x in rows:report+=f"{x['name']:<42} {x['cpu_peak_percent']:6.2f}%  {x['seconds']:6.1f}s  PASS\n"
report+='''
Warm Biome runs are 12-second smoke tests, not a substitute for careful listening
or long generative playback. No subjective claim of perfect sound follows from
CPU/RMS checks. The earlier broader twelve-engine matrix remains historical;
see Validation/Hub-Audit-2026-10-25 and the previous report for its exact hashes.

SOFTWARE / DATA INTEGRITY
Native sanitized transfer checks passed current/legacy decoding, malformed
uploads, stopped/internal-clock guards, CRC/ranges, all 16/64 slots, overwrite
consent, backup hold and no implicit save. Actual 22-object-map tests exercised
130 simulated interrupted flash writes: whole old or whole new data, never a mix.
These are simulated interruptions, not deliberate hardware power-cut tests.
Full DSP catalogue and 10,000 boundary/command/reassignment checks passed.
The final actual WASM passed 19 pages, banks, legacy import and patch roundtrip.
JavaScript checks passed packed bytes, corrupted backups, serialized transfers,
patch CRC/readback, bank naming, disconnect handling and packaged script syntax.
An isolated UI regression verifies that rapid backup clicks cannot start two
transfers while pending controls drain, and errors release the busy state.

FEATURES / LIMITS
Twelve engines, 224 factory tones, 76 musical Biomes plus Blank, six assignable
tracks, shared mixer/granular/delay/room, 16-step phrases, MIDI input, 16 named
User Biomes and 64 User Sounds. Original selected Mutable algorithms and
Marbles/Tides/Warps-inspired helpers; not complete module replicas.
The live editor transfers JSON patches and saved .mddb banks. Browser and device
banks remain separate. It displays live parameters, not the device framebuffer.
User slot names appear on BANKS; LIVE still shows the factory-origin name.
No dedicated copy/delete, MPE, MIDI OUT, live MIDI recording, per-step ties/
velocity or 64-step arranger. ECO reconstructs some voices at half rate;
Elements has a shared mode budget; granular recording is mono PCM16.
CPU2 remains disabled pending its own qualification. No arbitrary firmware-path
or all-possible-patches guarantee. The newly built private browser installer has
package/syntax checks; the physical roundtrip used the tested Python installer.

REMAINING BEFORE PUBLIC RELEASE
Owner listening; interactive browser/permission and complete UI workflows;
extended cold/power sessions, physical TRS and longer MIDI/jitter tests.
Local-file browser automation was denied; it was not bypassed. Protocol and
isolated-JS tests do not replace that remaining interactive test.
Any later firmware change requires qualification of its own exact binary.
Website/video/YouTube/Instagram and Sven follow after firmware acceptance.

LEFT READY
The user's original unsaved live patch from Before-Live-Editor was restored
byte-for-byte. The saved startup project, preferences and all banks remain equal
to the original complete backup. HOME, stopped, internal clock, physical master
control released. No test patch was saved over the startup project.
Published 1.0 and the developer handoff ZIP retain their recorded hashes.
No new public upload, publication or email was performed.

Audio-FM1-Audit contains earlier e59b USB recordings, not recordings of 427a.
Screens/Organic-Motion.gif and prior visual tests retain their provenance.
See Recovery-1.1.txt, LIVE-EDITOR-PROTOCOL.txt and RELEASE-1.1-PREPARATION.txt.
'''
(docs/'DEVICE-TEST-REPORT.txt').write_text(report)
p=docs/'REQUIREMENTS-AUDIT.txt';s=p.read_text()
s=s.replace('LIMIT: browser banks and hardware banks are separate. Live browser-to-device\nediting, full user-bank restore/import, dedicated delete/copy management and\npersistent user titles on LIVE are not currently complete.', 'YES: live browser-to-device editing, complete live patch JSON upload/readback,\nand full saved-bank .mddb backup/restore are integrated and device-tested.\nRestore requires physical SEL; saved bank writes require STOP/internal clock.\nLIMIT: browser banks and hardware banks are separate. Dedicated delete/copy\nmanagement and persistent user titles on LIVE are not currently complete.')
s=s.replace('OPEN: exact final binary from stock V15 and back, longer cold/power-cycle test,\nowner listening, physical TRS MIDI and extended release qualification.\nThe current device started this audit on our development firmware, not V15.', 'YES: exact 427a candidate -> original Stock V15 -> same candidate, followed\nby physically confirmed restore and byte-identical readback of all 22 objects.\nStock V15 erases User Banks/preferences: a complete .mddb backup is required.\nOPEN: longer cold/power sessions, owner listening, physical TRS MIDI and\ninteractive browser GUI/permission qualification. See current device report.')
s=s.replace('Update English website/manual/editor and install/return files; retain 1.0.', 'Private English editor, source and install/return files are prepared; retain 1.0.\nUpdate the public English website/manual after qualification and acceptance.')
s=s.replace('24–25 October 2026. Development candidate;', '25 October 2026, after live-editor/recovery integration. Development candidate;')
p.write_text(s)
(docs/'BANK-RESTORE-INTEGRATION.txt').write_text('''BANK BACKUP / RESTORE — IMPLEMENTED IN 1.1 DEVELOPMENT
25 October 2026. See DEVICE-TEST-REPORT.txt for the exact qualified package.

Enabled backup=true; Mutable Dreams hooks in app/transfer.c and app/app.h.
Platform A/B staging and journal retained; app-specific validation added before
staging. Twenty-two objects: 0,2,3..22. Magic MDDB, version 1. Calibration excluded.
Full bank transfer includes the SAVED startup project and preferences, 16 named
User Biomes and 64 User Sounds. It never silently saves the live patch.
Export unsaved live changes separately as JSON. Granular audio is not saved.

Backup blocks racing saves and can stream during playback. Restore requires
stopped/internal-clock state and physical SEL; remote keys cannot approve it.
Native validation accepts known legacy saved projects without changing audio
state, checks current banks/names/ranges, and refuses unknown future formats.
The journal at 0xE1000 shares the updater staging area; the main loop makes
these operations mutually exclusive. The final linked pool has 9264 bytes spare.

Native actual-map suite: 130 simulated write interruptions, CRC/truncation/wrong
app/cancellation checks, whole-old-or-whole-new recovery and calibration intact.
App-specific native tests cover all 16/64 slots, overwrite guards and holds.
Real physical SEL restore and all 22-object readback passed on final 427a.
Live browser JavaScript was also tested against the actual FM-1 via a MIDI
bridge. The complete interactive browser UI test remains a release gate.

Exact 427a -> Stock V15 -> 427a passed. V15 erased banks/preferences; restoring
the .mddb image recovered them byte-for-byte. Unsaved live edits were restored
separately. Keep backups private and retain the package alongside them.

Use web/device-ui.js with Connect FM-1, tools/dreams_backup.py, and
tools/device_editor.py. Read LIVE-EDITOR-PROTOCOL.txt and Recovery-1.1.txt.
Older read-only project_backup.py exports are retained for provenance, but do
not replace the complete .mddb format for the supported restore workflow.
''')
p=docs/'RELEASE-1.1-PREPARATION.txt';s=p.read_text().replace('- Finish exact 1.1 -> manufacturer V15 -> same 1.1 recovery qualification and\n  compare every saved bank/preference byte after restoration.', '- DONE for 427a: exact 1.1 -> manufacturer V15 -> same 1.1, physically confirmed\n  restore and byte-identical saved objects. Repeat if the final binary changes.\n  V15 erased banks/preferences; keep that backup requirement prominent.')
p.write_text(s)
(out/'START-HERE.txt').write_text('''MUTABLE DREAMS 1.1 — PRIVATE DEVELOPMENT PREVIEW
25 October 2026. Published 1.0 remains unchanged.

Open Mutable-Dreams-1.1.html. Start the instrument for independent browser audio,
or Connect FM-1 for the hardware editor. Hardware mode uses Web MIDI SysEx in
a supported browser on a secure page; listen through the FM-1. Its canvas shows
live parameters, not a mirrored hardware screen. Browser audio pauses while
connected to avoid two instruments playing together. Disconnect returns to it.

LISTENING
224 factory tones, twelve assignable engines, 76 musical Biomes plus Blank.
Start with Mellow Canopy, Satin River and Velour Dawn. Try the filter, slow
envelopes, bass phrases, shared delay/room and different Granular Size settings.
Adjust listening volume first. GLO > LOOK can freeze decorative animation.
Hold a function key for help. Marbles needs EUCLID/STEP phrases; Tides a filter.

CREATE AND KEEP YOUR OWN BIOME
Start with Blank or any factory Biome; choose all six sounds, rhythms, phrases,
mix and effects. Stop and choose internal clock before saving.
GLO > USER BANKS: K2 Biomes/sounds, K1 slot, K4 name cursor, PRESET character.
16 Biome slots and 64 sound slots; names up to 23 ASCII characters.
On the device SAVE twice within three seconds confirms an occupied slot;
in the live editor use the name field and Save user slot, then its confirmation.
Browser mode keeps its own local banks and uses repeat-Save confirmation.
K3 clockwise or Load user slot recalls a slot. Sound recall retains the target
rhythm/notes/mix; Biome recall restores all six tracks and their phrases.
Granular recordings and session MIDI routing are not saved in Biomes.
The BANKS page shows your custom name; LIVE retains the factory-origin title.

FILES AND RECOVERY
Export/Import patch exchanges one live Biome as JSON with the selected target.
Import changes the live patch; explicitly save to keep it on the device.
For older JSON formats, load into browser mode and export the converted patch
before importing it into the hardware editor.
Back up device banks downloads .mddb: SAVED project/preferences/all User Banks.
It does not save unsaved live edits. Export those separately as JSON.
Restore device banks replaces all saved objects and clears slots empty in the
backup. Stop, select internal clock, choose the file, then press physical SEL
when the FM-1 asks. The browser cannot approve this protection for you.

IMPORTANT: Stock V15 erased User Banks and preferences in our roundtrip test.
Keep a complete .mddb backup before returning to V15; restore after reinstalling
1.1. Read Recovery-1.1.txt. The private browser installer is
Mutable-Dreams-1.1-Install.html; the source and Python installer are also included.
The exact hardware roundtrip used the Python installer. Interactive browser
installation and full UI/permission workflows still need their own final test.

QUALIFICATION
Current installed package, exact hashes, measured loads and remaining gates:
DEVICE-TEST-REPORT.txt. The final build passed full restore/readback, live editor
protocol and simultaneous six-track audio/editing/backup tests. This is not public
release approval. Owner listening, longer cold/power sessions, physical TRS and
interactive browser tests remain. Original live patch and saved banks restored.

Audio/ contains older offline renders; Audio-FM1/ older device captures;
Audio-FM1-Audit/ holds e59b hardware recordings, not final 427a recordings.
All evidence retains its real hash. Screens/Organic-Motion.gif shows firmware
graphics from the previous UI checkpoint. The physical display remains 240x240.
RELEASE-1.1-PREPARATION.txt records the professional real-FM-1 video, calm English
female narration, community thanks, website, YouTube, Instagram and Sven plan.
Nothing new has been published or sent.
''')
for name in ['DEVICE-TEST-REPORT.txt','REQUIREMENTS-AUDIT.txt','BANK-RESTORE-INTEGRATION.txt','LIVE-EDITOR-PROTOCOL.txt','Recovery-1.1.txt','RELEASE-1.1-PREPARATION.txt']:
 shutil.copy2(docs/name,out/name)
shutil.copytree(history,out/'Validation/Live-Editor-2026-10-25',dirs_exist_ok=True)
shutil.copy2(fw,out/'Mutable-Dreams-1.1.0-dev.fwsc')
subprocess.run([sys.executable,str(src/'tools/package_development_installer.py'),str(fw),str(out/'Mutable-Dreams-1.1-Install.html'),str(r/'editor-qualification.json')],check=True)
print('PASS exact binary qualification and reports prepared. Installer/script/package verification and final sealing follow.')
