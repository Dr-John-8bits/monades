# Monades

*Six voix. Chacune suit son propre principe ; toutes restent accordées.*

**Monades** est un firmware génératif pour le **M-VAVE FM-1**, dérivé de [Mutable Dreams 1.1](https://github.com/kidbee85-beep/mutable-dreams) de **Kid Bunka / Christoph Link**.

Le nom vient de la *Monadologie* de Leibniz (1714). Une monade est une unité qui évolue selon son principe interne, sans « fenêtre » sur les autres, et pourtant toutes s'accordent par une harmonie préétablie. Ici, ce sont six voix génératives qui suivent chacune leur propre logique, mais partagent la même gamme et le même temps.

**Site** : [dr-john-8bits.github.io/monades](https://dr-john-8bits.github.io/monades/), avec le simulateur dans le navigateur, le [mode d'emploi](https://dr-john-8bits.github.io/monades/manuel/), les [nouveautés](https://dr-john-8bits.github.io/monades/nouveautes/), le [MIDI](https://dr-john-8bits.github.io/monades/midi/) et la [feuille de route](https://dr-john-8bits.github.io/monades/feuille-de-route/).

> Version 0.1.2 (30 octobre 2026). Monades est **installé et lancé sur un FM-1 depuis le 29 octobre 2026** (depuis la V15) : il démarre et joue. La 0.1.1 a corrigé l'audio USB, qui craquait sous la charge ; la 0.1.2 ajoute la cible LFO ROTATE (le rythme euclidien tourne tout seul). Les vérifications complètes ([docs/monades/TESTS-FM1.md](docs/monades/TESTS-FM1.md)) sont en cours. Pour un instrument plus éprouvé, utilisez [Mutable Dreams](https://github.com/kidbee85-beep/mutable-dreams).

## Installer
Sur le site, section [Installer](https://dr-john-8bits.github.io/monades/#installer) : depuis le navigateur (Chrome ou Edge, [installeur](https://dr-john-8bits.github.io/monades/installer/)) ou depuis le Mac :

```sh
python3 -m pip install mido python-rtmidi
python3 base/tools/fm1_install.py monades-0.1.2.fwsc --yes
```

Aucune manipulation sur le FM-1 : allumé sur la V15 ou sur Mutable Dreams (pas sur la V16 : des problèmes d'installation ont été signalés depuis la V16) et relié en USB, il passe tout seul en mode mise à jour. Retour au V15 : `python3 base/tools/fm1_install.py FM-1-V15.fwsc --yes` (reconnu à son empreinte, sans `--force`). Liste des vérifications sur l'appareil : [docs/monades/TESTS-FM1.md](docs/monades/TESTS-FM1.md).

## Ce que Monades ajoute à Mutable Dreams
Le détail est sur la page [Nouveautés](https://dr-john-8bits.github.io/monades/nouveautes/) et dans [docs/monades/FORK.md](docs/monades/FORK.md). En résumé :
- **Écrans** : chaque page montre ce qu'on règle (vumètres, oscilloscope, courbes de filtre et d'enveloppe, bande granulaire, piano-roll).
- **Harmonie** : 91 gammes en 15 familles, dont 12 microtonales (Scala), ragas avec gamakas et notes pivots, gamme CUSTOM.
- **Génératif** : générateurs aléatoire, Markov et motif, curseur déjà-vu, forme en sections, pas à pas.
- **Modulation et effets** : deux LFO par piste (19 cibles), filtres HP, BP et notch, chorus, écho synchronisé, diffusion.
- **MIDI** : sortie des notes générées par USB (par piste ou par zones de notes), canaux d'entrée et de sortie au choix pour chaque piste, horloge en sortie.
- **Mondes** : 14 Mondes FDDS, DICE sur la page affichée, morphing d'un Monde à l'autre, dérive.

Les recherches de départ (matériel, autres firmwares, code Mutable Instruments) sont dans [docs/monades/RECHERCHE.md](docs/monades/RECHERCHE.md).

## Compiler
Comme pour Mutable Dreams : voir [BUILDING.md](BUILDING.md). Le build Mac (`python tools/build_local.py host`) ne demande pas de FM-1. Pour le FM-1 sur un Mac Apple Silicon (chaîne JieLi sous l'émulateur Blink, SDK AC79), l'environnement utilisé le 28 octobre 2026 est décrit dans [docs/monades/FORK.md](docs/monades/FORK.md) (« Première compilation pour le FM-1 ») ; `tools/package_monades_installer.py` refait ensuite l'installeur du site.

## Simulateur dans le navigateur
Le firmware complet, avec le panneau du FM-1 dessiné (celui de fm1-base) : mêmes boutons, molettes et touches, même écran, même son. Il faut le [WASI SDK 34](https://github.com/WebAssembly/wasi-sdk/releases/tag/wasi-sdk-34) :

```sh
export WASI_SDK=/chemin/vers/wasi-sdk-34.0-arm64-macos
python tools/build_local.py web
python tools/package_emu.py
python -m http.server 8100 --directory app/build/emu
```

Puis ouvrir `http://localhost:8100/`. La page est `app/web/emu/index.html` (MPL-2.0, d'après fm1-base). Le site en publie une copie : après un changement du firmware, refaire `python tools/package_emu.py site/simulateur`.

## Audio USB
- **Enregistrer le FM-1 dans l'ordinateur par le câble USB** : activé (`usb_audio_in = true` dans `app/fm1.toml`), 44,1 kHz stéréo, le mélange final (les pistes ne sortent pas séparément). Mutable Dreams 1.1 l'a vérifié sur un FM-1.
- **Faire jouer l'ordinateur à travers le FM-1** (le FM-1 comme carte son) : désactivé, comme dans Mutable Dreams (`usb_audio_out = false`). La plateforme sait le faire : il suffit de passer cette ligne à `true` et de recompiler. Cela prend de la mémoire et du calcul à un moteur déjà chargé ; à essayer seulement sur l'appareil, en surveillant la page SYSTEM.

## Identité
- Identité du firmware : préfixe `FM-1_714` (distinct de Mutable Dreams `FM-1_710`).
- USB : « Monades Project » / « Monades FM-1 ». Pas de Bluetooth MIDI (`ble = false`).

## Crédits et licence
- **Mutable Dreams** : Kid Bunka / Christoph Link. Le README d'origine est conservé dans [README-MUTABLE-DREAMS.md](README-MUTABLE-DREAMS.md).
- **Mutable Instruments** : Émilie Gillet (Plaits, Rings, Elements, stmlib, sous licence MIT).
- **fm1-base** (Charles Vestal) et **Felucca / felucca-bsp** (Leo Kuroshita).

Distribution combinée sous **GPL-3.0-only**. Les notices MIT, MPL-2.0 et des polices sont conservées (voir [LICENSING-GPL.txt](LICENSING-GPL.txt)). Projet indépendant, sans lien avec Mutable Instruments, M-VAVE ni l'auteur de Mutable Dreams, et non approuvé par eux.
