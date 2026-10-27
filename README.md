# Monades

*Six voix. Chacune suit son propre principe ; toutes restent accordées.*

**Monades** est un firmware génératif pour le **M-VAVE FM-1**, dérivé de [Mutable Dreams 1.1](https://github.com/kidbee85-beep/mutable-dreams) de **Kid Bunka / Christoph Link**.

Le nom vient de la *Monadologie* de Leibniz (1714). Une monade est une unité qui évolue selon son principe interne, sans « fenêtre » sur les autres, et pourtant toutes s'accordent par une harmonie préétablie. Ici, ce sont six voix génératives qui suivent chacune leur propre logique, mais partagent la même gamme et le même temps.

> ⚠️ Projet en tout début de développement (0.1). Pas encore de version à flasher. Pour un instrument fini, utilisez [Mutable Dreams](https://github.com/kidbee85-beep/mutable-dreams).

## Ce qui est prévu
La feuille de route complète est dans [docs/monades/FORK.md](docs/monades/FORK.md). En résumé :
- **Lisibilité** : chaque écran montre ce qu'on règle (vumètres réels, oscilloscope, courbes de filtre et d'enveloppe exactes, granulaire visualisé).
- **MIDI** : notes générées envoyées en MIDI, avec un **routage par zones de notes vers des canaux**, et l'horloge MIDI en sortie.
- **Harmonie** : sélecteur de gammes exhaustif, Scala et microtonal, ragas, notes saisies par l'utilisateur.
- **Génératif** : Markov, déjà-vu façon Marbles, phrases qui mutent, structure de morceau.
- **Tempo et effets** : LFO synchronisés avec plusieurs cibles (S&H compris), filtres HP/BP/Notch, delay synchronisé, vraie réverbe, chorus.

Les recherches de départ (matériel, autres firmwares, code Mutable Instruments) sont dans [docs/monades/RECHERCHE.md](docs/monades/RECHERCHE.md).

## Compiler
Comme pour Mutable Dreams : voir [BUILDING.md](BUILDING.md). Le build Mac (`python tools/build_local.py host`) ne demande pas de FM-1.

## Simulateur dans le navigateur
Le firmware complet, avec le panneau du FM-1 dessiné (celui de fm1-base) : mêmes boutons, molettes et touches, même écran, même son. Il faut le [WASI SDK 34](https://github.com/WebAssembly/wasi-sdk/releases/tag/wasi-sdk-34) :

```sh
export WASI_SDK=/chemin/vers/wasi-sdk-34.0-arm64-macos
python tools/build_local.py web
python tools/package_emu.py
python -m http.server 8100 --directory app/build/emu
```

Puis ouvrir `http://localhost:8100/`. La page est `app/web/emu/index.html` (MPL-2.0, d'après fm1-base).

## Audio USB
- **Enregistrer le FM-1 dans l'ordinateur par le câble USB** : activé (`usb_audio_in = true` dans `app/fm1.toml`), 44,1 kHz stéréo. Mutable Dreams 1.1 l'a vérifié sur un FM-1.
- **Faire jouer l'ordinateur à travers le FM-1** (le FM-1 comme carte son) : désactivé, comme dans Mutable Dreams (`usb_audio_out = false`). La plateforme sait le faire : il suffit de passer cette ligne à `true` et de recompiler. Cela prend de la mémoire et du calcul à un moteur déjà chargé ; à essayer seulement sur l'appareil, en surveillant la page SYSTEM.

## Identité
- Identité du firmware : préfixe `FM-1_714` (distinct de Mutable Dreams `FM-1_710`).
- USB : « Monades Project » / « Monades FM-1 ».

## Crédits et licence
- **Mutable Dreams** : Kid Bunka / Christoph Link. Le README d'origine est conservé dans [README-MUTABLE-DREAMS.md](README-MUTABLE-DREAMS.md).
- **Mutable Instruments** : Émilie Gillet (Plaits, Rings, Elements, stmlib, sous licence MIT).
- **fm1-base** (Charles Vestal) et **Felucca / felucca-bsp** (Leo Kuroshita).

Distribution combinée sous **GPL-3.0-only**. Les notices MIT, MPL-2.0 et des polices sont conservées (voir [LICENSING-GPL.txt](LICENSING-GPL.txt)). Projet indépendant, sans lien avec Mutable Instruments, M-VAVE ni l'auteur de Mutable Dreams, et non approuvé par eux.
