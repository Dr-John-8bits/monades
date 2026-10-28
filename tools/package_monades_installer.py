#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The Monades browser installer: base/web/installer.html (fm1-base / Felucca), in French with English one click away,
the package embedded and checked against its SHA-256, the identity confirmed after the restart (as Mutable Dreams 1.1).
   python3 tools/package_monades_installer.py app/build/monades-0.1.0.fwsc site/installer
writes site/installer/index.html and a copy of the package for the Mac updater (base/tools/fm1_install.py)."""
from pathlib import Path
import base64,hashlib,json,re,shutil,sys
R=Path(__file__).resolve().parents[1]
if len(sys.argv)!=3:raise SystemExit('Usage: package_monades_installer.py PACKAGE OUTDIR')
pkg,out=Path(sys.argv[1]),Path(sys.argv[2]);raw=pkg.read_bytes()
product=''.join(chr((raw[i*48+47]-i-1)&255) for i in range(20) if raw[i*48+47]!=0x7d)
assert product=='FM-1_714000100',product
assert b'FELUCCA-LOADER-1' in raw,'no Felucca update loader in this package'
version='0.1.0'
s=(R/'base/web/installer.html').read_text()
values={'NAME':'Monades','REPO':'https://github.com/Dr-John-8bits/monades','REPO_SHORT':'github.com/Dr-John-8bits/monades',
        'LICENSE':'GPL-3.0 ; notices MIT et MPL-2.0 conservées','CREDITS':' Fork de Mutable Dreams (Kid Bunka). Plaits, Rings, Elements : Émilie Gillet / Mutable Instruments.'}
for k,v in values.items():s=s.replace('{{'+k+'}}',v)
def strip(x):return re.sub(r'^import .*?;\n','',re.sub(r'^export\s+','',x,flags=re.M),flags=re.M)
lib='\n'.join(strip((R/'base/web'/n).read_text()) for n in ['fm1pkg.js','fm1ota.js'])
meta={'version':version,'product':product,'sha256':hashlib.sha256(raw).hexdigest(),'data':base64.b64encode(raw).decode()}
s=s.replace('/*LIB*/',lib).replace('/*META*/',json.dumps(meta))
def rep(a,b):
 global s
 assert s.count(a)==1,a[:70];s=s.replace(a,b)
# The package travels inside the page and is checked before anything is written (as the 1.1 release installer).
rep('const r = await fetch(meta.pkg);\n    if (!r.ok) throw new Error(`${meta.pkg}: HTTP ${r.status}`);\n    const pkg = new Uint8Array(await r.arrayBuffer());', '''const pkg = Uint8Array.from(atob(meta.data), c => c.charCodeAt(0));
    const digest = [...new Uint8Array(await crypto.subtle.digest('SHA-256', pkg))].map(b=>b.toString(16).padStart(2,'0')).join('');
    if(digest !== meta.sha256) { sayK('badpkg'); return; }''')
rep('    const raw = new Uint8Array(await f.arrayBuffer());','    if(f.size>2000000) { sayK("stockbad"); return; }\n    const raw = new Uint8Array(await f.arrayBuffer());')
rep('async function run(img, product, isStock) {\n  let midi;', '''async function run(img, product, isStock) {
  if(writing || !img)return;
  writing=true;
  $("go").disabled=$("stockgo").disabled=$("reportgo").disabled=true;
  let midi;''')
rep('catch (e) { sayK("denied"); return; }','catch (e) { writing=false; $("go").disabled=!image; $("stockgo").disabled=!stock; $("reportgo").disabled=false; sayK("denied"); return; }')
rep('sayK((await up.resume(img, step)) ? (isStock ? "stockdone" : "done") : "stopped");', '''if(!(await up.resume(img, step)))throw fail('stopped','The write did not finish.');
      sayK('reboot');
      await sleep(3000);
      const back=await up.waitFor(id=>!IS_OTA(id),40000);
      if(!back)throw fail('noreturn','The FM-1 did not restart.');
      back.link.close();
      if(back.id.text!==product)throw fail('mismatch','Unexpected firmware after restart.',back.id.text);
      $('bar').value=100;sayK(isStock?'stockdone':'done');''')
rep('if (isStock && e && e.code === "mismatch") { $("bar").value = 100; sayK("stockdone"); }   /* stock names itself its own way */\n    else if', 'if')
rep('  writing = false;\n  $("go").disabled', '  writing = false;\n  $("reportgo").disabled=false;\n  $("go").disabled')
# French first, English one click away (in place of Japanese).
fr='''  fr: {
    intro: "Relier le FM-1 en USB, puis appuyer sur Installer. Fonctionne dans Chrome ou Edge ; le navigateur demande une fois l'accès au MIDI : l'accepter.",
    newtab: " (s'ouvre dans un nouvel onglet)", progress: "Progression", pct: ", {n} %",
    warn: "Ne pas débrancher le câble USB pendant l'écriture.",
    install: "Installer", other: "English",
    manual: "Retour au site de Monades",
    direct: "Relier le câble USB directement à l'ordinateur. Aucun autre matériel n'est nécessaire.",
    brick: "Si le FM-1 ne démarre plus, un Mac peut remettre le firmware d'origine :",
    bricklink: "remettre en marche un FM-1 qui ne démarre plus (en anglais)",
    back: "Pour revenir au firmware officiel : « Revenir au firmware d'origine » plus bas, ou l'outil de M-VAVE, M-UPGRADE (Downloads → PC Software → M-UPGRADE).",
    stockh: "Revenir au firmware d'origine",
    stockwhy: "Écrit le FM-1 V15 de M-VAVE à la place de Monades (ou de son mode sans échec), par le même câble USB. Le retour au V15 efface les banques utilisateur et les préférences : en garder une sauvegarde .mddb avant.",
    stock1: "Télécharger le FM-1 V15 de M-VAVE :",
    stock2: "Choisir le fichier téléchargé :",
    stock3: "Relier le FM-1 en USB et appuyer sur le bouton ci-dessous.",
    stockinstall: "Installer le firmware d'origine",
    stockok: "C'est bien le FM-1 V15 de M-VAVE. Appuyer sur le bouton pour l'installer.",
    stockbad: "Ce n'est pas le fichier FM-1 V15 de M-VAVE. Choisir le fichier du lien ci-dessus.",
    stockdone: "Terminé. Le FM-1 a redémarré avec son firmware d'origine.",
    nomidi: "Ce navigateur ne peut pas le faire. Ouvrir la page dans Chrome ou Edge.", loadfail: "Chargement impossible : ",
    start: "Démarrage", verify: "Vérification", loader: "Passage en mode mise à jour", write: "Écriture",
    reboot: "Redémarrage", done: "Terminé", stopped: "Arrêté. Rebrancher le câble USB et appuyer de nouveau sur Installer.",
    error: "Erreur : ",
    denied: "L'accès au MIDI (SysEx) n'a pas été autorisé. L'autoriser dans les réglages du site (barre d'adresse) et appuyer de nouveau sur Installer.",
    notfound: "FM-1 introuvable. Vérifier le câble USB (un câble de données), fermer les autres applications qui utilisent le MIDI, et appuyer de nouveau sur Installer.",
    model: "L'appareil branché n'est pas un FM-1.",
    lost: "La liaison USB a été perdue. Rebrancher et appuyer de nouveau sur Installer.",
    noloader: "Le FM-1 en mode mise à jour est introuvable. Rebrancher le câble USB et appuyer de nouveau sur Installer (il reste en mode mise à jour jusqu'à la fin).",
    noreturn: "Le FM-1 n'est pas revenu. L'éteindre et le rallumer.",
    mismatch: "Écrit, mais le FM-1 annonce une autre version : ",
    badpkg: "Le fichier à installer est abîmé. Recharger la page.",
    leave: "Écriture en cours. Ne pas fermer la page.",
    license: "{{LICENSE}}, sans garantie. Sources :",
    reporth: "Signaler un problème",
    reportwhy: "Si le FM-1 a planté, ne pas l'éteindre : le relier en USB et appuyer sur le bouton ci-dessous. Le rapport de plantage survit à un redémarrage, mais pas à l'extinction. Copier ce qui s'affiche et l'envoyer, avec ce qu'on faisait à ce moment-là.",
    reportread: "Lire le rapport de plantage", reportcopy: "Copier", reportlabel: "Rapport de plantage",
    reportwait: "Lecture…", reportnone: "Pas de réponse. Vérifier que le FM-1 fait tourner Monades.",
    reportok: "Lu. Le copier et l'envoyer.", reportcopied: "Copié.",
  },
'''.replace('{{LICENSE}}',values['LICENSE'])
s,n=re.subn(r'  ja: \{.*?\n  \},\n(?=  en: \{)',lambda m:fr,s,flags=re.S);assert n==1
rep('install: "Install", other: "日本語",','install: "Install", other: "Français",')
rep('let lang = "en";                                 /* English first; 日本語 is a click away */','let lang = "fr";                                 /* French first; English is a click away */')
rep('$("lang").lang = lang === "ja" ? "en" : "ja";','$("lang").lang = lang === "fr" ? "en" : "fr";')
rep('$("lang").addEventListener("click", () => { lang = lang === "ja" ? "en" : "ja"; applyLang(); });','$("lang").addEventListener("click", () => { lang = lang === "fr" ? "en" : "fr"; applyLang(); });')
rep('<button type="button" class="lang" id="lang" lang="ja">日本語</button>','<button type="button" class="lang" id="lang" lang="en">English</button>')
rep('<html lang="en">','<html lang="fr">')
rep('<title>Install · Monades</title>','<title>Installer Monades</title>')
rep('<h1>Install Monades</h1>',f'<h1>Installer Monades {version}</h1>\n  <p class="small">Pas encore testé sur un FM-1. Identité après installation&nbsp;: <code>{product}</code>. Avant de changer de firmware, garder une sauvegarde <code>.mddb</code> de son travail.</p>\n  <p class="small">Partir du firmware d\'origine V15 ou de Mutable Dreams (avec M-VAVE Open, remettre d\'abord le V15). Aucune manipulation sur le FM-1, aucune combinaison de touches&nbsp;: allumé sur son firmware habituel et relié en USB, il passe tout seul en mode mise à jour, puis redémarre.</p>')
rep(' (or its safe mode, or another firmware with the same update entry)', '')
rep('What Monades saved may be lost.', 'Returning to V15 erases the User Banks and preferences. Back up .mddb first.')
assert not re.search(r'\{\{[A-Z_]+\}\}',s) and '日本語' not in s
out.mkdir(parents=True,exist_ok=True)
(out/'index.html').write_text(s)
shutil.copy(pkg,out/pkg.name)
shutil.copy(R/'base/tools/fm1_install.py',out/'fm1_install.py') # the Mac updater (MPL-2.0, its header kept)
print(out/'index.html',product,meta['sha256'])
