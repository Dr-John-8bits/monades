from pathlib import Path
import hashlib,json,shutil,zipfile
root=Path.cwd();src=root/'work/mutable-dreams-next';out=root/'outputs/Mutable-Dreams-1.1-Mellow-Preview';r=root/'work/device-mellow'
private=root/'outputs/Mutable-Dreams-Private-Backup-2026-10-24';docs=src/'docs/development'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
q=json.loads((r/'editor-qualification.json').read_text());assert q['pass_'] and not q['public_release_approved']
assert 'PASS private installer' in (r/'editor-installer-test.log').read_text()
assert sha(src/'app/build/mutable-dreams-1.1.0-dev.fwsc')==q['firmware_sha256']==sha(out/'Mutable-Dreams-1.1.0-dev.fwsc')
for folder in [docs/'mellow-validation/live-editor-2026-10-25',out/'Validation/Live-Editor-2026-10-25']:
 for name in ['editor-installer-test.log','finish-editor-state.log']:shutil.copy2(r/name,folder/name)
 # Preserve the scripts used to generate reports; they do not contain bank data.
 for name in ['finalize_editor_checkpoint.py','seal_editor_checkpoint.py']:shutil.copy2(r/name,folder/name)
 (folder/'RUNNING-TESTS.txt').write_text('''This folder preserves executed reports and local bench harnesses, not a turnkey
hardware test command. Local bench scripts reference workspace-relative paths
and a private baseline .mddb image, which is deliberately excluded. Before a new
hardware qualification, make fresh backups of that device and adapt paths. Do
not run a flashing/restore harness against an unfamiliar instrument blindly.

Portable software tests live in tests/ and base/tests/ in the complete source.
Build sanitized host objects, then tools/test_transfer.py; build web WASM, package
the page, run tests/web_engine.mjs, tests/device_editor.mjs and the private
installer tests. Consult the source README and build repair notes.
Physical restore confirmation must come from the operator pressing SEL.
''')
# Mark older summaries as historical without changing their numerical evidence.
for base in [docs,out]:
 p=base/'1.1-MELLOW-CHECKPOINT.txt';s=p.read_text()
 if not s.startswith('HISTORICAL'):
  p.write_text('HISTORICAL MELLOW CHECKPOINT — retained for provenance.\nThe current state is DEVICE-TEST-REPORT.txt and Recovery-1.1.txt.\nLive editor, full restore and exact V15 roundtrip were completed subsequently.\n\n'+s)
def source_files():
 for p in sorted(src.rglob('*')):
  rel=p.relative_to(src)
  if p.is_file() and not any(x in rel.parts for x in ['build','__pycache__','.git']) and p.name!='.DS_Store':
   assert p.suffix not in ['.mddb','.raw','.fwsc'],f'Unexpected firmware/private dump in source: {rel}'
   yield p,rel
manifest=''.join(f'{sha(p)}  {rel.as_posix()}\n' for p,rel in source_files() if p.name!='SOURCE-SHA256.txt')
(src/'SOURCE-SHA256.txt').write_text(manifest)
archive=out/'Mutable-Dreams-1.1-Mellow-Source.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,6) as z:
 for p,rel in source_files():z.write(p,'mutable-dreams-next/'+rel.as_posix())
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None
 for line in manifest.splitlines():
  h,n=line.split('  ',1);assert hashlib.sha256(z.read('mutable-dreams-next/'+n)).hexdigest()==h,n
with zipfile.ZipFile(out/'Mutable-Dreams-1.1-Installer-Kit.zip') as z:assert z.testzip() is None
# No private images are present in the distributable preview tree.
assert not any(p.suffix in ['.mddb','.raw'] for p in out.rglob('*') if p.is_file())
for item in json.loads((r/'editor-protected-artifacts.json').read_text()):assert sha(root/item['file'])==item['sha256']
def write_sums(folder):
 p=folder/'SHA256SUMS.txt'
 p.write_text(''.join(f'{sha(f)}  {f.relative_to(folder).as_posix()}\n' for f in sorted(folder.rglob('*')) if f.is_file() and f.name not in ['SHA256SUMS.txt','.DS_Store']))
write_sums(out);write_sums(private)
resume=dict(date='2026-10-25',firmware_sha256=q['firmware_sha256'],device=q['device'],development_only=True,preview=str(out),backup=str(private),status='Live editor/full restore implemented; exact V15 roundtrip and physical bank restore/readback passed; device stress passed. Original live and saved state restored; stopped/HOME/internal clock/physical master. Public release pending remaining gates.',final_peak=q['peak_callback_percent'],preexisting_xruns=8,stress_xrun_increase=0,remaining=q['remaining'],published_1_0_unchanged=True,source_zip=str(archive),source_zip_sha256=sha(archive))
(r/'RESUME.json').write_text(json.dumps(resume,indent=2))
(r/'TRANSFER-PROGRESS.json').write_text(json.dumps(dict(date='2026-10-25',status='Completed and sealed for owner testing, not published',current_device=q['firmware_sha256'],verified_backup=str(private/'Live-Editor/Final-Editor.mddb'),original_live=str(private/'Before-Live-Editor/Mutable-Dreams-Live.raw'),all_saved_objects_restored=True,original_live_restored=True,remaining=q['remaining']),indent=2))
with (root/'PROJEKTSTAND.txt').open('a') as f:
 f.write('\n25 October 2026 — Live editor and full bank restore completed and saved.\nFWSC '+q['firmware_sha256']+'\nExact candidate -> Stock V15 -> candidate passed; user physically confirmed restore. All 22 saved objects and original unsaved live patch byte-identical. V15 ERASES User Banks/preferences; .mddb backup required.\nSix-track editor/backup stress peak '+f"{q['peak_callback_percent']:.2f}"+'%; zero new faults during measured playback. Eight pre-existing timing events retained in report for startup/restore investigation.\nPrivate preview/editor/installer/source/recovery and English release/video/Sven brief saved. No public changes. 1.0 and handoff unchanged.\nFM-1: HOME, stopped, internal clock, physical master; original live patch restored.\nResume: work/device-mellow/RESUME.json\n')
print(json.dumps(dict(status='SEALED',firmware_sha256=q['firmware_sha256'],source_zip_sha256=sha(archive),source_files=len(manifest.splitlines()),private_backups_verified=True,published_1_0_unchanged=True,remaining=q['remaining']),indent=2))
