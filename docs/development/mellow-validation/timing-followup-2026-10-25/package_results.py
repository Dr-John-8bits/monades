"""Package observed evidence only; never flash, publish or modify user banks."""
from pathlib import Path
import json,hashlib,shutil,zipfile
root=Path.cwd();r=root/'work/timing-audit';src=root/'work/mutable-dreams-next';docs=src/'docs/development'
out=root/'outputs/Mutable-Dreams-1.1-Mellow-Preview';private=root/'outputs/Mutable-Dreams-Private-Backup-2026-10-25-Timing'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
H='427a34eedd2cc625b7833f3f953436bc1db1845f94cde695d8eed47332d09534'
assert sha(src/'app/build/mutable-dreams-1.1.0-dev.fwsc')==H==sha(out/'Mutable-Dreams-1.1.0-dev.fwsc')
initial=json.loads((r/'initial.json').read_text());cancel=json.loads((r/'restore-timeout.json').read_text());cold=json.loads((r/'cold-start.json').read_text())
long=json.loads((r/'long-audio-stress.json').read_text());midi=json.loads((r/'midi-long.json').read_text());final=json.loads((r/'final-state.json').read_text())
assert all(x['status']['xruns']==8 for x in initial)
assert cancel['cancelled_by_timeout'] and cancel['all_saved_objects_unchanged'] and cancel['xrun_delta']==3
assert cold['status']['xruns']==0 and cold['all_22_saved_objects_exact']
assert len(long)==5 and all(not x['issues'] and x['baseline']['xruns']==0 and x['final']['xruns']==0 and x['cpu_peak_percent']<85 for x in long)
assert midi['after_clock_help']['xruns']==0 and midi['messages']>=64000
assert all(final[k] for k in ['all_22_saved_objects_exact','latest_user_live_restored','stopped','home','physical_master_released','internal_clock'])
checks=json.loads((r/'manual-browser.json').read_text()) if (r/'manual-browser.json').exists() else dict(browser_only='Owner confirmed audio, page/knob controls and user-slot save/load',live_gui='Pending')
report=f'''MUTABLE DREAMS 1.1 — TIMING FOLLOW-UP AND EXTENDED QUALIFICATION
25 October 2026. Private development; published 1.0 unchanged.
Exact installed firmware: {H}
No firmware code or binary changed during this investigation. The previous
qualified Stock V15 roundtrip applies to this same package.

THE EIGHT PRE-EXISTING TIMING EVENTS
The lifetime xrun counter began at 8. It stayed 8 through fresh live snapshots,
two complete backups, six complete live-patch upload/readbacks, all 19 pages,
three factory Biome recalls and restoration of the post-listening live patch.

We then sent the same full bank backup for restore and deliberately allowed
physical confirmation to time out. This stages inactive flash copies but does
not commit replacements. With transport stopped and no notes triggered, the
counter rose from 8 to 11. The 22 active saved objects remained byte-identical.
Peak callback time stayed 3511 us (budget 5804 us), overload count stayed zero.
The platform log reported a 17325 us interrupt-service gap; the longest app
critical section was only 41 us. No crash was recorded. A retained watchdog
history line is not evidence of a new reset: uptime increased through the test.

Source corroboration: base/bsp/chip/fm1_flash.h fl_erase4k/fl_write disable
interrupts while the SPI NOR erase/program operation completes, because code
executes from that same flash. base/device/unity.c st_erase zeroes the audio
buffer before the erase. main_fm1.c increments the xrun counter if DMA advances
while servicing a delayed half-buffer interrupt. After a flash pause, a render
can start late and cross the DMA boundary even though its own compute time is
well below budget. This explains the reproduced stopped-transfer events.

The original eight events have no individual timestamps, so each cannot be
assigned retrospectively. The same category is now directly reproduced during
storage maintenance, not during ordinary playback or editor reads. No evidence
of a sustained DSP overload was found. We did not suppress, relabel or manually
zero counters in firmware. A real power cycle naturally begins a fresh counter.
Saving/restoring remains a stopped/internal-clock operation and may pause or
silence audio tails. This is not supported as seamless real-time flash writing.

COLD START AND PERSISTENCE
The owner physically powered the FM-1 off/disconnected USB and restarted it.
The fresh boot reported 0 xruns and 0 overloads. The saved startup project and
every saved preference/User Bank matched the fresh pre-test backup exactly.

EXTENDED PLAYBACK — EXACT SAME BINARY
The following windows started and ended at zero xruns. No new overloads,
nonfinite samples, dropped commands, automatic quality reductions or host USB
capture overruns occurred. Every stress track triggered. The two dense cases
changed pages/parameters and backed up every saved bank during playback.
CPU below is peak callback time / 5804 us, not total system utilization.

'''
for x in long:report+=f"{x['name']:<43} {x['seconds']:6.1f}s  {x['cpu_peak_percent']:6.2f}%  PASS\n"
report+=f'''
USB MIDI
{midi['messages']} note/CC/note-off messages over two minutes on six channels.
Each channel received all {midi['cycles']} scheduled Note Ons. USB audio was
finite, audible and complete. No new fault counters during the MIDI test or
subsequent clock/help tests. Peak callback {midi['after_midi']['callback_peak_us']} us.
Actual USB Start/Stop/Continue and 24-PPQN clock passed; clock loss stopped
transport after two seconds. USB clock was ignored when TRS clock was selected
(routing test only). All fourteen held-help/release-without-tap checks passed.
Physical TRS cabling was not tested: the owner has USB available and explicitly
deferred the TRS test. USB success does not certify physical TRS wiring.

USER BANK SAVE / FINAL STATE
An existing User Biome was loaded and saved three times using precisely its
existing name and payload. No invented patch replaced personal content.
Storage-related timing deltas for those stopped writes:
{[x['xrun_delta'] for x in final['same_payload_resave_tests']]}
Readback matched. The complete final backup again equals the original backup.
The user's actual live patch after their listening session was restored exactly;
the older pre-listening patch was not substituted. HOME, stopped, internal clock,
physical master and keys released. Private backups remain outside the release.

BROWSER EVIDENCE AND LIMITS
The local-file URL policy rejected automated page access and explicitly forbade
workarounds. We did not use an alternate browser surface or indirect execution
to evade it. Isolated JS tests passed binary/CRC/queue/transfer/error handling,
packaged page syntax and installer package/identity/restart checks. Prior real
MIDI tests used the shipped JS client and verified its device protocol.
Owner browser-only result: {checks['browser_only']}
Owner live connected GUI/permission result: {checks['live_gui']}
These human observations apply to the reported actions. They do not automatically
certify every file dialog, layout, failure path or browser installation workflow.
Read BROWSER-ACCEPTANCE.txt for the remaining visible/permission/installer steps.

RELEASE BOUNDARY
Finite stress tests and one physical cold start do not certify all patches or
every power-failure scenario. Interrupted-write atomicity remains supported by
the separate simulated 130-write-position suite and real restore/readback.
The Python installer V15 roundtrip passed previously on this exact binary; a real
browser installer roundtrip is not claimed. Public 1.0/handoff hashes unchanged.
No publication, email or new video upload occurred.
The separate plugin study runs in its own chat, reading the firmware only.
'''
(docs/'TIMING-AND-EXTENDED-TESTS.txt').write_text(report)
v=docs/'mellow-validation/timing-followup-2026-10-25';v.mkdir(parents=True,exist_ok=True)
names=['initial.json','initial.log','restore-timeout.json','restore-timeout.log','after-staging-platform.log','cold-start.json','cold-start.log','long-audio-stress.json','longrun.log','midi-long.json','midi-long.log','final-state.json','storage-and-finish.log','browser-protocol.log','browser-installer.log','initial.py','restore_timeout.py','cold_readback.py','longrun.py','midi_long.py','storage_and_finish.py','package_results.py']
if (r/'manual-browser.json').exists():names.append('manual-browser.json')
for n in names:shutil.copy2(r/n,v/n)
(v/'PROVENANCE.txt').write_text('All device tests in this folder used 427a34ee (full hash in report).\nPrivate .mddb/raw/live patch exports deliberately excluded. Bench scripts need\na fresh local private baseline; adapt paths before use. No firmware code changed.\n')
p=docs/'DEVICE-TEST-REPORT.txt';s=p.read_text();header='LATEST FOLLOW-UP: TIMING-AND-EXTENDED-TESTS.txt\n25 October: storage timing reproduced, physical cold start verified, longer\naudio/MIDI tests completed, browser owner observations recorded. Read that\nreport for current remaining gates; the earlier detailed evidence follows.\n\n'
if not s.startswith('LATEST FOLLOW-UP:'):s=header+s
s=s.replace('earlier timing events cannot be attributed conclusively. They are not concealed\nor relabelled as zero: localize them during extended startup/restore qualification.','earlier individual events were not timestamped. The subsequent investigation\nreproduced storage-maintenance timing events while stopped; see the follow-up\nreport for direct evidence and zero-xrun playback after a physical cold start.')
s=s.replace('Owner listening; interactive browser/permission and complete UI workflows;\nextended cold/power sessions, physical TRS and longer MIDI/jitter tests.', 'Owner listening and the remaining visible browser/permission/installer steps;\nphysical TRS (owner deferred), end-to-end latency and longer jitter/power testing.\nPhysical cold start, extended playback and 64,800 USB messages passed in the\nfollow-up. Browser-only sound, controls and User Bank save/load were confirmed\nby the owner. See TIMING-AND-EXTENDED-TESTS.txt for current exact scope.')
s=s.replace("The user's original unsaved live patch from Before-Live-Editor was restored\nbyte-for-byte. The saved startup project, preferences and all banks remain equal\nto the original complete backup.", "At the earlier checkpoint, the Before-Live-Editor patch was restored. The new\nfollow-up instead preserved and restored the user's more recent post-listening\nlive patch byte-for-byte. Saved project, preferences and banks match the fresh\n25 October timing-audit backup.")
p.write_text(s)
p=docs/'REQUIREMENTS-AUDIT.txt';s=p.read_text().replace('OPEN: longer cold/power sessions, owner listening, physical TRS MIDI and\ninteractive browser GUI/permission qualification. See current device report.', 'YES: physical cold start with complete saved-data persistence and extended\nzero-xrun audio/MIDI windows; storage timing investigated separately.\nOwner confirmed browser-only audio, controls and User Bank save/load.\nOPEN: physical TRS MIDI (owner deferred), remaining visible browser/permission\nsteps and browser installer qualification. See the follow-up test report.');p.write_text(s)
for n in ['TIMING-AND-EXTENDED-TESTS.txt','BROWSER-ACCEPTANCE.txt','DEVICE-TEST-REPORT.txt','REQUIREMENTS-AUDIT.txt']:shutil.copy2(docs/n,out/n)
shutil.copytree(v,out/'Validation/Timing-Followup-2026-10-25',dirs_exist_ok=True)
p=out/'START-HERE.txt';s=p.read_text();note='\nLATEST: Read TIMING-AND-EXTENDED-TESTS.txt for the storage timing explanation,\nphysical cold start, extended zero-xrun audio/MIDI tests and owner browser check.\nBROWSER-ACCEPTANCE.txt separates confirmed actions from remaining manual steps.\n'
s=s.replace('release approval. Owner listening, longer cold/power sessions, physical TRS and\ninteractive browser tests remain. Original live patch and saved banks restored.', 'release approval. Physical cold start and longer audio/USB-MIDI tests passed.\nOwner confirmed browser-only audio, controls and User Bank save/load. Physical\nTRS, remaining visible browser/installer steps and owner listening remain. The\nlatest post-listening live patch and all saved banks were preserved.')
if note not in s:s+=note
p.write_text(s)
def files():
 for p in sorted(src.rglob('*')):
  rel=p.relative_to(src)
  if p.is_file() and not any(x in rel.parts for x in ['build','__pycache__','.git']) and p.name!='.DS_Store':
   assert p.suffix not in ['.raw','.mddb','.fwsc'];yield p,rel
(src/'SOURCE-SHA256.txt').write_text(''.join(f'{sha(p)}  {rel.as_posix()}\n' for p,rel in files() if p.name!='SOURCE-SHA256.txt'))
with zipfile.ZipFile(out/'Mutable-Dreams-1.1-Mellow-Source.zip','w',zipfile.ZIP_DEFLATED,6) as z:
 for p,rel in files():z.write(p,'mutable-dreams-next/'+rel.as_posix())
with zipfile.ZipFile(out/'Mutable-Dreams-1.1-Mellow-Source.zip') as z:
 assert z.testzip() is None
 for line in (src/'SOURCE-SHA256.txt').read_text().splitlines():
  h,n=line.split('  ',1);assert hashlib.sha256(z.read('mutable-dreams-next/'+n)).hexdigest()==h
for p,h in [('outputs/Mutable-Dreams-1.0.fwsc','75008881683cbeb3515e24cfd7830136b90eb293f0752dc66b203b7f2ee7cd9f'),('outputs/Mutable-Dreams-Developer-Handoff-2026-10-24.zip','630e34663b3935db22a353cf781ab591ca438a1b82cfa1854e526bc361944c55')]:assert sha(root/p)==h
for folder in [out,private]:
 (folder/'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.relative_to(folder).as_posix()}\n' for p in sorted(folder.rglob('*')) if p.is_file() and p.name not in ['SHA256SUMS.txt','.DS_Store']))
resume=json.loads((r/'RESUME.json').read_text());resume.update(status='Bench tests completed and source/report bundle sealed; remaining manual browser/installer and physical TRS checks are explicit, device restored to post-listening state',running=None,source_sha256=sha(out/'Mutable-Dreams-1.1-Mellow-Source.zip'),browser=checks,next=['Physical TRS when equipment available','Remaining manual browser/permission/installer actions','Owner listening/acceptance then public 1.1 website/video/YouTube/Instagram/Sven work']);(r/'RESUME.json').write_text(json.dumps(resume,indent=2))
p=root/'work/device-mellow/RESUME.json';j=json.loads(p.read_text());j.update(status=resume['status'],followup_resume=str(r/'RESUME.json'),timing='Reproduced stopped flash-maintenance timing; cold boot and extended playback have zero xruns',remaining=resume['next'],plugin_chat=resume['plugin_chat']);p.write_text(json.dumps(j,indent=2))
project_note=('\n25 October 2026 — Timing follow-up on unchanged 427a firmware.\nStopped restore staging reproduced 3 timing events, with 17.325 ms interrupt gap; all data unchanged. Physical cold start and extended playback/MIDI tested. Post-listening live state and all saved data restored.\nEvidence: outputs/Mutable-Dreams-1.1-Mellow-Preview/TIMING-AND-EXTENDED-TESTS.txt\nResume: work/timing-audit/RESUME.json. Physical TRS deferred by owner; browser automated access blocked, owner observations recorded. Published 1.0/handoff unchanged.\nPlugin feasibility study created in separate chat 01a17030-f889-74f3-ba5f-c46599214361. No public changes.\n')
project_state=root/'PROJEKTSTAND.txt'
if project_note not in project_state.read_text():
 with project_state.open('a') as f:f.write(project_note)
print(json.dumps(dict(status='SEALED',firmware_sha256=H,long_audio_peak=max(x['cpu_peak_percent'] for x in long),midi_messages=midi['messages'],all_user_data_preserved=True,browser=checks,published_1_0_unchanged=True),indent=2))
