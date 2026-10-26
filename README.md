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

## Identité
- Identité du firmware : préfixe `FM-1_714` (distinct de Mutable Dreams `FM-1_710`).
- USB : « Monades Project » / « Monades FM-1 ».

## Crédits et licence
- **Mutable Dreams** : Kid Bunka / Christoph Link. Le README d'origine est conservé dans [README-MUTABLE-DREAMS.md](README-MUTABLE-DREAMS.md).
- **Mutable Instruments** : Émilie Gillet (Plaits, Rings, Elements, stmlib, sous licence MIT).
- **fm1-base** (Charles Vestal) et **Felucca / felucca-bsp** (Leo Kuroshita).

Distribution combinée sous **GPL-3.0-only**. Les notices MIT, MPL-2.0 et des polices sont conservées (voir [LICENSING-GPL.txt](LICENSING-GPL.txt)). Projet indépendant, sans lien avec Mutable Instruments, M-VAVE ni l'auteur de Mutable Dreams, et non approuvé par eux.
