# Synthé génératif sur mesure — enquête (octobre 2026)

## 1. Le M-VAVE FM-1 sous le capot

| Élément | Ce qu'on sait |
|---|---|
| SoC | JieLi **AC791N (WL82)**, 2 cœurs **pi32v2** (dérivé Blackfin), 240 MHz |
| Mémoire | Flash 1 Mo (un seul slot appli, < 600 Ko). RAM ~512 Ko (chiffre communautaire, non confirmé) |
| Audio | 44 118 Hz, blocs de 64 échantillons, moteur d'origine en **virgule fixe**. Codec non documenté |
| Contrôles | 27 touches en silicone (vélocité lue via l'ADC par le projet *Velo*), 4 potentiomètres, encodeurs, écran couleur 240×240 |
| Connectique | USB-C (USB MIDI + **USB audio class-compliant**), MIDI IN en TRS, BLE MIDI, **pas de MIDI OUT TRS** (sortie en USB/BLE seulement) |
| Toolchain | JieLi PI32v2 Clang 4.0.1 (Windows), SDK AC79 doc en chinois, pas de port de debug |
| Flash | Installateur web (Chrome/Edge), récupération via le ROM bootloader « UBOOT1.00 », sans ouvrir le boîtier |

**Limites concrètes** : un seul firmware à la fois, ~500 Ko pour les samples, budget CPU serré (Felucca coupe des voix au-delà de 85 % ; X0X dégrade la qualité sous charge). FM-106 : 6 voix ≈ 30 % des deux cœurs.

## 2. L'écosystème des firmwares alternatifs

Environ 90 firmwares open source (majoritairement GPL-3.0), presque tous dérivés de **Felucca** (Leo Kuroshita) et de sa couche matérielle **felucca-bsp** (MPL-2.0).

**Bases pour développer**
- **Felucca** : multi-moteurs, USB MIDI, USB audio, chargeur de mise à jour, installateur web. Moteurs dans *Felucca-Engines* (MIT, C99 virgule fixe).
- **Dinghy** (zvenson) : template minimal (USB, panneau, écran documentés).
- **foma** : un firmware en un seul fichier C, compilé dans Docker et testé dans un émulateur WebAssembly.
- **Émulateurs** : fm1-emulator (Rust), fm1-emu (AU/VST3) → on peut développer sans flasher.
- **fm1_open** (officiel M-VAVE) : appli GPL-3.0, mais **moteur sonore en binaire fermé**.

**Génératifs**
- **AMB-1** (Charles Vestal) : machine ambient autonome dès l'allumage, boucles « tape » et percussions euclidiennes.
- **HYP-1** (techno/acid), **MIN-1** (ensemble minimaliste), **IND-1** (industriel en 6 couches).
- **MAKU** : un nouveau monde aléatoire dans la tonalité à chaque démarrage.
- **Unholy Union** : drone qui arrange pad, séquence et arpège.
- **GammaWave** : générateur de morceaux sur 49 genres.
- **Melodee (Tess)** : générateurs algorithmiques « Tessera » sur 4 pistes, + **gammes microtonales**, + entrée USB audio 4 canaux.
- **DriftSeq** : générateurs euclidiens/probabilistes (pas encore flashable).

**Ports Mutable Instruments**
- **Hortator** : modèles de batterie Plaits + **Grids**. La preuve qu'un port est possible.
- **Lunar Modulator** : basé sur Plaits/Braids/Rings, au stade recherche (jouable dans le navigateur, pas encore sur la machine).
- **Mutable Dreams 1.0** (Kid Bunka / Christoph Link, 18 oct. 2026, GPL-3.0) : **le plus proche de ce projet**, génératif *et* Mutable. 6 pistes assignables, 8 moteurs (extraits de Plaits : batteries, FM, VA, harmonique ; résonateur modal de Rings ; un « Choir » maison), 64 « Biomes » (mondes génératifs éditables), 128 presets. Euclidien, probabilités, swing, accents, répétitions, gammes partagées, filtres par piste, bus granulaire (freeze, reverse, transpose), 4 macros, instrument navigateur qui fait tourner le vrai moteur. L'auteur précise : « Selected algorithms, not complete module ports ». Le mode ECO s'enclenche tout seul sous forte charge, ce qui confirme le plafond CPU. La doc ne dit rien des réverbe/delay/chorus, du MIDI ou de l'USB audio. Toolchain : JieLi pi32v2 + SDK AC79 ; build web avec WASI SDK. Dépôt : https://github.com/kidbee85-beep/mutable-dreams

**Fermés** : Baud Girl FM-1+VA, Groove OS.

## 3. Reprendre du code Mutable Instruments

**Licence** : le code STM32 de `pichenettes/eurorack` est sous **MIT** (le code AVR, plus ancien, est en GPL-3.0). Le matériel est en CC-BY-SA et le nom « Mutable Instruments » est une marque déposée. On peut donc tout réutiliser, à condition de citer et de ne pas reprendre le nom.

**Modules les plus utiles pour ce projet**

| Module | Intérêt |
|---|---|
| **Marbles** | *Le* module génératif : générateurs aléatoires t/x, « déjà-vu » (boucle ↔ aléatoire en continu), quantificateur qui **apprend des gammes**, contrôle de la distribution (spread/bias/steps) |
| **Grids** | Carte de patterns de batterie interpolée (X/Y + densité) |
| **Plaits** | 16 à 24 modèles de synthèse, dont le FM 6-op compatible DX7 |
| **Rings / Elements** | Résonateurs modaux et physiques, réverbe |
| **Clouds / Beads** | Granulaire + réverbe Dattorro |
| **stmlib** | Briques DSP (filtres SVF, interpolations, `FxEngine` pour réverbe/chorus/diffuseur) |

**Correction (26/10, après lecture du code de Mutable Dreams)** : le code Plaits/Rings/Elements tourne **en float sans conversion** sur le pi32v2 (compilé en `-O3`). La contrainte réelle est le budget CPU (pics à 85 % du temps disponible avec 6 voix sur un seul cœur), pas le format numérique. Le paragraphe ci-dessous est donc trop pessimiste.

**Le vrai problème sur le FM-1** : le code DSP de Mutable (Plaits, Rings, Clouds, Marbles) est écrit en **float** pour Cortex-M4F/M7 avec FPU. Le pi32v2 tourne en virgule fixe (tous les moteurs FM-1 le sont). Porter Plaits demande donc de le **convertir en virgule fixe**, ou d'accepter un float émulé très lent. C'est faisable pour un ou deux modèles (Hortator l'a fait pour la batterie), mais pas pour Rings ou Clouds en entier avec seulement ~500 Ko de RAM.

En revanche, **la partie générative ne coûte presque rien en CPU** : la logique de Marbles ou de Grids (aléatoire, Markov, déjà-vu, quantification) se porte facilement, même en virgule fixe.

## 4. Les besoins un par un

| Besoin | Sur FM-1 (firmware perso) | Sur hardware maison |
|---|---|---|
| Génératif « vrai » (Markov, déjà-vu, automates, L-systems, mutation) | ✅ très faisable, peu de CPU | ✅ |
| Toutes les gammes (ragas, maqams, slendro/pelog, microtonal, Scala `.scl`) | ✅ (Melodee le fait déjà ; API de tuning dans fm1_open) | ✅ |
| Sortie notes MIDI | ⚠️ USB/BLE seulement, pas de TRS OUT | ✅ TRS IN + OUT + USB |
| USB audio | ✅ déjà présent (Felucca, Dinghy, FM-106) | ✅ |
| Belle réverbe stéréo (type Clouds/Dattorro) | ⚠️ limitée par la RAM et le CPU | ✅ |
| Delay, chorus, modulations | ✅ modestes | ✅ |
| Moteurs Mutable complets | ❌ trop lourd, sauf portages partiels | ✅ |

**Sur les ragas** : un raga, ce n'est pas qu'une gamme. Il y a aussi des phrases ascendantes et descendantes différentes (*arohana/avarohana*), des notes pivots (*vadi/samvadi*), des ornements (*gamakas*, glissandos) et un accordage en *shrutis* (22 divisions). Un moteur génératif qui connaît ces règles (par exemple une chaîne de Markov par raga plus des glissandos) va bien plus loin qu'un simple quantificateur.

## 5. Plateformes pour un synthé maison

| Plateforme | CPU / RAM | USB audio + MIDI | Écosystème Mutable | Commentaire |
|---|---|---|---|---|
| **Electrosmith Daisy Seed** (~30 €) | STM32H750 480 MHz FPU, 64 Mo SDRAM, codec 24 bits | USB MIDI ✅, USB audio : communautaire | **Énorme** (Plaitsy, ports Rings/Clouds, DaisySP inspiré de MI) | Le choix le plus naturel pour réutiliser Mutable |
| **Teensy 4.1** (~35 €) | Cortex-M7 600 MHz, FPU, PSRAM optionnelle | **USB audio + USB MIDI natifs**, simples | Ports possibles (Braids/Plaits existent) | Le plus simple pour l'USB audio |
| **Raspberry Pi Pico 2 (RP2350)** (~6 €) | 2 × M33 150 MHz, FPU simple précision | TinyUSB (UAC2 + MIDI) | Partiel | Pas cher, mais réverbe de qualité difficile |
| **Raspberry Pi 4/5 / CM5** | Linux, plusieurs GHz | ✅ | Tout (VCV Rack / Cardinal contient tous les modules Mutable) | Démarrage lent, consommation, moins « instrument » |

## 6. Recommandation

**Les deux pistes, dans cet ordre :**

1. **Phase 1 : firmware perso sur le FM-1** (peu cher, rapide). Le meilleur point de départ est sans doute de forker **Mutable Dreams**, qui a déjà le génératif, des moteurs Plaits/Rings portés, le granulaire et une build navigateur. Autres options : **Melodee** (gammes microtonales) ou **Dinghy**/**foma** pour repartir de zéro. Y ajouter :
   - un moteur génératif façon **Marbles** (déjà-vu, spread/bias, quantificateur) + **Grids** pour la batterie ;
   - une bibliothèque de gammes importée du format **Scala** (plus de 5 000 gammes libres) + des « grammaires » de ragas ;
   - la sortie MIDI USB/BLE pour piloter la MPC.
   → On apprend la contrainte embarquée, et on teste les idées génératives à la main.

2. **Phase 2 : instrument maison sur Daisy Seed ou Teensy 4.1** quand les idées sont validées. Là on aura la place pour : moteurs Plaits/Rings en float sans conversion, réverbe Clouds/Dattorro stéréo, MIDI TRS IN/OUT, USB audio, potards et écran choisis selon tes besoins. Le cœur génératif écrit en C++ portable pendant la phase 1 se réutilise tel quel.

**Conseil d'architecture** : écrire le moteur génératif et les gammes comme une **bibliothèque C/C++ indépendante du matériel** (sans malloc, en virgule fixe ou avec un type numérique abstrait), testable sur le Mac (et en plugin VST). Elle pourra ensuite tourner sur le FM-1, sur Daisy ou sur Teensy.

## Sources
- The Firmware Hub — https://thefirmwarehub.com/
- awesome-fm-1 — https://github.com/cicloid/awesome-fm-1
- fm1-custom-fw (hub, teardown, RE) — https://github.com/aroum/fm1-custom-fw
- FM-1-RE — https://github.com/AL-255/FM-1-RE
- Felucca — https://github.com/hugelton/Felucca
- Melodee — https://github.com/keremimo/melodee
- fm1_open (officiel) — https://github.com/m-vave/fm1_open
- Inside the M-VAVE FM-1 — https://thesynthsource.com/inside-the-m-vave-fm-1/
- Mutable Instruments eurorack (code) — https://github.com/pichenettes/eurorack
- Plaitsy (port Plaits sur Daisy) — https://forum.electro-smith.com/t/plaitsy-plaits-port-for-patch-mutable-daisies/8982

## 7. Idées pour le fork (cahier des charges)

### Sortie MIDI filtrée des notes générées
Envoyer en MIDI une partie des notes générées par le moteur, pour piloter la MPC ou un autre synthé.
- **Table de routage par zones de notes** (le « top du top ») : une liste de zones, chacune avec une note basse, une note haute et un canal MIDI. Exemple :
  | Zone | Plage | Canal |
  |---|---|---|
  | 1 | C1 → B2 | 1 (basse) |
  | 2 | C3 → B4 | 2 (pad/accords) |
  | 3 | C5 → C7 | 3 (lead) |
  - 8 zones environ (suffisant, et la table tient en quelques octets).
  - Les zones peuvent se chevaucher : une note dans deux zones part sur les deux canaux (doublage).
  - Les bornes sont-elles incluses ? À trancher. Suggestion : basse incluse, haute incluse, et l'éditeur prévient en cas de chevauchement non voulu.
  - Option par zone : transposition (±octaves) et filtre de source (toutes les pistes ou seulement certaines).
  - Table globale par défaut, sauvegardée avec le Biome. Réglable sur l'appareil et dans l'éditeur navigateur (plus confortable pour 8 zones).
- Mode simple en plus : **par piste**, MIDI OUT activé ou non, avec un canal fixe (1-16).
- Option : envoyer seulement en MIDI (piste muette en interne) ou en MIDI + son interne.
- Option : la vélocité suit les accents ; la note-off est envoyée selon la durée ou le gate de la note.
- Plus tard : filtrer aussi par probabilité (« une note sur N »), par degré de la gamme (seulement les fondamentales…), ou en transposant d'une octave.
- Envoyer l'horloge MIDI (clock out + start/stop) pour que la MPC suive le tempo.

**Faisabilité** : la plateforme sait déjà envoyer du MIDI (`plat_midi_send` en USB et BLE dans `base/include/plat.h`). Mutable Dreams ne s'en sert simplement pas. Le point d'insertion est l'endroit où le séquenceur déclenche une note (`fire()` dans `app/dsp/dreams.cpp`). Il faut y ajouter une petite file de note-off. Pas de MIDI OUT en jack TRS sur le FM-1 : la sortie passe par USB (ou Bluetooth).

### Gammes : sélecteur exhaustif
- Remplacer les 8 gammes actuelles (`md_quantize`, masque 12 bits) par une bibliothèque complète : modes occidentaux, gammes jazz et bebop, maqams, ragas (thaats et ragas avec arohana/avarohana différents), gammes japonaises, gamelan (slendro/pelog), gammes symétriques…
- Microtonal : import de fichiers **Scala `.scl`/`.kbm`**, notes non entières (cents) pour les moteurs internes ; en sortie MIDI, approximation par pitch bend par canal (ou MPE plus tard).
- Navigation par familles sur l'appareil (Occident / Inde / Moyen-Orient / Asie / Microtonal…), recherche dans l'éditeur navigateur.

### Notes saisies par l'utilisateur
- Pouvoir **taper ses propres notes** (un ensemble de notes, ou une phrase ordonnée) que le moteur joue selon le séquenceur programmé : rythme euclidien, probabilités, variations.
- Deux usages : un **ensemble de hauteurs** (« joue seulement C, Eb, G, Bb » : une gamme maison) et une **mélodie/motif** que le générateur parcourt, varie ou mute.
- Saisie au clavier du FM-1 (mode enregistrement pas à pas), par MIDI entrant, ou dans l'éditeur navigateur.
- Point de départ existant : les phrases de 16 pas de Mutable Dreams (`md_extension.notes[16]`, décalages en demi-tons ou silences).

### Synchronisation au tempo
- Delay synchronisé au tempo (1/4, 1/8 pointée, triolets…).
- Modulations « Tides » synchronisables (cycles en mesures plutôt qu'en secondes).
