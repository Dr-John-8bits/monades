# Monades — feuille de route (fork de Mutable Dreams)

Base : Mutable Dreams 1.1 (Kid Bunka / Christoph Link), GPL-3.0-only.
Obligations : garder les notices (MIT Mutable Instruments, MPL felucca-bsp, GPL), publier les sources de chaque binaire distribué, créditer l'original. Changer le nom, l'identité USB (`app/fm1.toml`) et le préfixe d'ID firmware pour ne pas se confondre avec l'original.

## Principes de design
- **Toute animation doit visualiser ce qu'on règle.** Pas d'animation purement décorative : chaque mouvement à l'écran reflète un paramètre ou l'état réel du moteur (grains, LFO, pas du séquenceur, enveloppes…). Tourner un potard doit se *voir* immédiatement. Sinon on ne comprend plus ce qu'on fait.
- L'option sans animation reste disponible, mais elle garde l'information (version statique).

## Nom
**Choisi : Monades** (d'après la *Monadologie* de Leibniz, 1714). Préfixe firmware `FM-1_714`. Pistes écartées :
| Nom | Idée |
|---|---|
| **Dhow** | Voilier de l'océan Indien, la route des ragas ; suit la tradition nautique de l'écosystème (Felucca, Sloop, Skiff, Ketch, Dinghy) |
| **Alap** | Ouverture improvisée et sans pulsation d'un raga : du génératif pur |
| **Tanpura** | Le bourdon qui porte le raga |
| **Rhizome** | Prolonge l'esprit « Biome » : un réseau qui pousse tout seul |
| **Marée** | Clin d'œil à Tides, cycles qui reviennent sans jamais être identiques |

Vérifier qu'aucun firmware FM-1 ne porte déjà le nom (The Firmware Hub, awesome-fm-1).

## Ajouts

### Lot 1 — MIDI (peu de CPU, gros gain d'usage)
1. **Sortie MIDI des notes générées** en USB (et BLE), via `plat_midi_send`, avec une file de note-off.
2. **Routage par zones** : jusqu'à 8 zones (note basse, note haute, canal), par ex. C1–B2 → canal 1, C3–B4 → canal 2. Chevauchements autorisés (doublage) avec une alerte dans l'éditeur. Options par zone : transposition par octaves, filtre de pistes sources.
3. Mode simple : MIDI OUT par piste avec un canal fixe. Option « MIDI seul » (piste muette en interne).
4. **Horloge MIDI en sortie** (clock + start/stop) pour que la MPC suive.
5. Vélocité de sortie qui suit les accents.
   **Fait (branche `monades/midi-out`)** : 1 (USB ; pas de Bluetooth dans cette version, `ble = false`), 2 (8 zones, chevauchements = note sur chaque canal ; page MIDI OUT ZONES), 3 (TRACKS 1-6), 4 (clock 24 ppqn, start / stop), 5 (vélocité des accents). Pitch bend des gammes microtonales en mode TRACKS.
   **Reste** : transposition et filtre de pistes par zone, option « MIDI seul ».

### Lot 2 — Harmonie
6. **Sélecteur de gammes exhaustif** : modes, jazz/bebop, maqams, thaats et ragas, gammes japonaises, gamelan, gammes symétriques… rangés par familles.
7. **Import Scala `.scl`/`.kbm`** et microtonal : hauteurs en cents pour les moteurs internes, pitch bend par canal en sortie MIDI.
8. **Ragas complets** : montée et descente différentes (arohana/avarohana), notes pivots (vadi/samvadi), ornements (gamakas, glissandos).
9. **Saisie de tes propres notes** : soit un ensemble de notes (gamme maison), soit un motif que le générateur parcourt et fait varier. Saisie au clavier, par MIDI entrant ou dans l'éditeur navigateur.
   **Fait** : 91 gammes par familles, montée / descente des ragas, gamme CUSTOM au clavier, microtonal et Scala `.scl` (branches précédentes). Puis, branche `monades/raga`, page RAGA / GAMAKA (cycle GLO, après HARMONY) :
   - **vadi / samvadi** : deux notes de la gamme qui attirent les notes générées (un tiers des notes à deux demi-tons ou moins) et sont accentuées ;
   - **gamakas** par piste : **kan** (note de grâce depuis la note du dessus dans la gamme), **meend** (glissé depuis la note précédente), **andolan** (balancement lent vers la note du dessous), **mix** (un des trois par note, tiré de la graine), avec une quantité qui règle durée et profondeur ;
   - **motif au clavier** sur la page GENERATOR : chaque touche écrit sa note dans la phrase et avance, REC efface. Nouvelle source **MOTIF** qui parcourt le motif dans l'ordre, SPREAD = part de variation ; MARKOV l'apprend aussi ;
   - patch v9 (1676 octets), migration depuis v8, v7, 1.1 et plus anciens.
   **Reste** : `.kbm`, chargement de Scala sans recompiler, saisie par MIDI entrant ou dans l'éditeur navigateur, gamakas en sortie MIDI (pitch bend).

### Lot 3 — Génératif « vrai »
10. Remplacer la variation actuelle (euclidien + probabilité + graine renouvelée tous les 32 pas) par un moteur à part et lisible :
    - chaînes de Markov apprises sur une phrase ou un raga ;
    - curseur « déjà-vu » façon Marbles (boucle figée ↔ aléatoire total) ;
    - phrases qui mutent lentement, et structure de morceau (sections, tension et détente) ;
    - boucles de longueurs inégales qui se décalent (à la Eno).
11. Garder Biomes, macros et rythmes euclidiens comme cadre.
   **Fait (branche `monades/generatif`)**, pages GENERATOR / DEJA VU et FORM (cycle SEQ, après PHRASE) :
   - par piste, une **source de notes** : OFF (comportement 1.1, macro Motion), **RANDOM** dans un ambitus (SPREAD, 0 à 24 demi-tons), ou **MARKOV** : chaîne d'ordre 1 apprise sur la phrase de la page PHRASE (une gamme de raga y est respectée, montée et descente comprises) ;
   - un **registre déjà-vu** façon Marbles, de 1 à 16 cases : à chaque coup, il rejoue sa case ou tire du nouveau. 0 = toujours neuf, 100 = boucle figée, entre les deux = phrase qui mute lentement. Il mémorise aussi les décisions de Chance, donc il fige le groove, même sur une piste de batterie ;
   - une **forme** globale : calme, montée, sommet, détente, de 2 à 16 mesures chacune. La tension fait varier la densité, et 0 à 3 pistes se reposent dans les sections calmes, d'autres à chaque cycle ;
   - les boucles de longueurs inégales (longueur 1 à 32 et division par piste) existaient déjà ;
   - patch v8 (1620 octets) ; les patchs v7, 1.1 et plus anciens se chargent avec tout cela coupé.
   **Reste** : Markov d'ordre 2, tension qui agit aussi sur le registre, le filtre ou les effets. (Le motif saisi au clavier est fait : lot 2, branche `monades/raga`.)

### Lot 4 — Tempo et effets
12. **Delay synchronisé au tempo** (1/4, 1/8 pointée, triolets).
13. **Tides synchronisable** (cycles en mesures).
14. **Vraie réverbe stéréo** (Dattorro ; la base est déjà dans `third_party` Clouds/Rings `fx_engine`) à la place de la petite « room ».
15. **Chorus / ensemble**.
15b. **Filtres en plus du passe-bas** : aujourd'hui OFF / LP12 / LP24 / LPG seulement. Ajouter **HP12, HP24, BP, Notch**, presque gratuits car le `stmlib::Svf` les calcule déjà (`dreams.cpp:373` est figé sur `FILTER_MODE_LOW_PASS`). **Validé** : un HP global sur les envois d'effets (réverbe et delay moins boueux).
15c. **LFO de filtre synchronisé au tempo** : aujourd'hui « Breathe » est un sinus de période fixe (7 s + 2,3 s par piste) et Tides tourne librement de 0,5 à 20 s (`dreams.cpp:352`), rien n'est synchronisé. Ajouter par piste :
   - un choix **libre / synchronisé**, avec des divisions (1/16 … 8 mesures, pointé, triolet) ;
   - des formes : sinus, triangle, dent de scie montante et descendante, carré, sample & hold (aléatoire en escalier, très génératif) ;
   - **S&H validé** ;
   - profondeur bipolaire (±), et **cibles multiples** (2 ou 3 destinations par LFO, chacune avec sa profondeur) :
     - **Filtre** : cutoff, résonance ;
     - **Enveloppe** : attaque, decay ;
     - **Timbre du moteur** : tone, harm, shape (les paramètres timbre/morph de Plaits, Rings, etc.) ;
     - **Hauteur** : vibrato léger, ou pas de gamme avec le S&H (notes aléatoires quantifiées) ;
     - **Volume et pan** : trémolo, auto-pan ;
     - **Effets** : envois réverbe, delay et granulaire, feedback du delay, position et taille du grain ;
     - **Génératif** : probabilité, densité (hits), Marbles. Faire « respirer » le pattern lui-même ;
   - calcul une fois par bloc de 64 échantillons : coût CPU négligeable ;
   - redémarrage de phase : libre, à chaque note, ou à chaque mesure.
   Garder le mode libre actuel comme option « organique ». Penser à la migration des patchs, car `md_validate` borne aujourd'hui `filter` à 0–3.
   **Fait (branche `monades/tempo-effets`)** :
   - filtres **HP12, BP, Notch** en plus de LP12 / LP24 / LPG ;
   - **passe-haut 12 dB sur les envois** echo et room (20 Hz à 2 kHz, OFF par défaut), **couleur de l'écho et de la room** (page SPACE TONE) ;
   - **deux LFO par piste** (pages LFO A et LFO B) : 15 divisions synchronisées (8 mesures … 1/32, pointées, triolets) qui redémarrent avec PLAY et suivent l'horloge MIDI, ou 16 vitesses libres de 0,02 à 20 Hz ; 7 formes dont S&H et aléatoire lissé, tirés de la graine ; profondeur ±. Cibles : cutoff, résonance, niveau (trémolo), pan, hauteur, tone, harmonics, shape, envois echo / room / grain, temps d'écho, position du grain ;
   - patchs v7 : les patchs, biomes utilisateur et sauvegardes de Mutable Dreams 1.1 se chargent avec tout cela au neutre.
   **Fait ensuite (branche `monades/effets-2`)** : HP24 ; cibles LFO attaque, decay, feedback de l'écho, taille du grain, **notes dans la gamme** (S&H = notes aléatoires quantifiées, aussi en sortie MIDI) et **hits** (densité) ; **écho synchronisé** (1/32 … 1/4, pointées, triolets ; divisé par deux tant qu'il dépasse les 371 ms de la ligne) ; **chorus** de Rings sur les pistes sèches ; **diffusion** Dattorro (4 allpass) devant la room. Page ECHO SYNC / CHORUS.
   **Pas fait, faute de mémoire** : la vraie réverbe Dattorro de Rings demande 64 Ko, et le « pool » du FM-1 n'avait que 9 Ko libres en 1.1 (le chorus en prend 4). Il faudrait raccourcir la bande granulaire (256 Ko) ou trouver de la place après mesure sur l'appareil. Tides synchronisé : couvert par les LFO synchronisés sur le cutoff. Second cœur : impossible sans la chaîne de compilation JieLi et l'appareil.
   **Reste** : plusieurs cibles par LFO, redémarrage à chaque note.
16. Activer le **second cœur** pour faire de la place (actuellement désactivé, pics à 85 % sur un cœur).

### Lot UI — Lisibilité : chaque écran montre ce qu'on règle
15d. **Page Granular réactive** : aujourd'hui l'animation (`app.c:308`) est décorative, 43 points pilotés uniquement par le temps. La remplacer par une vue de la bande :
   - la **bande** (environ 3 s de tampon) en bandeau horizontal, avec un résumé de la forme d'onde sur environ 200 colonnes (pics mis à jour au fil de l'écriture, pas de relecture des 131 072 échantillons) ;
   - la **tête d'écriture** qui défile, et qui s'arrête en **Freeze** ;
   - chaque **grain** dessiné à sa position de lecture, sa largeur suivant **Grain Size** et son éclat suivant son enveloppe ;
   - **Position** : la zone de lecture surlignée ;
   - **Feedback** : des traînées ou des échos qui persistent ;
   - **Space** : la densité ou l'intensité globale ; **Reverse** : grains qui remontent vers la gauche.
   Coût : quelques valeurs lues par image (écran rafraîchi toutes les 80 ms, dans la boucle principale et pas dans l'audio). Exposer l'état des grains via `md_status()` ou un nouvel accesseur.
15e. **Global Mixer lisible** (`app.c:238`) : aujourd'hui la « feuille » montre la position du réglage Level, le vrai vumètre n'est qu'un trait de 2 px qui sature vite (`track_peak*260`, plein vers −12 dBFS), et les envois Grain/Delay/Reverb ne sont que des chiffres, pour la piste sélectionnée seulement. À faire :
   - pour chaque piste, un **vumètre large en dB** (échelle logarithmique, mémoire de crête, rouge à l'écrêtage), avec le réglage Level marqué par un repère par-dessus ;
   - les **3 envois visibles pour les 6 pistes en même temps** (petites barres ou anneaux Grain/Delay/Reverb sous chaque piste) ;
   - des **vumètres de retour** pour le granulaire, le delay et la réverbe, plus un **vumètre master** ;
   - un indicateur visible quand le limiteur doux (`clip()`) agit ;
   - une piste mutée grisée, et une piste qui joue qui « pulse » sur ses coups.
   Coût : le moteur calcule déjà `track_peak` ; il faut juste ajouter les crêtes des bus d'effets et du master (une comparaison par échantillon).
15f. **Audit page par page** (lecture de `md_draw_all`, `app.c:180-335`). ✅ = déjà réactif, ⚠️ = partiel ou faux, ❌ = décoratif ou texte seul.

| Page | Aujourd'hui | Amélioration |
|---|---|---|
| **LIVE BIOME** | ⚠️ Les « fleurs » gonflent un peu avec le niveau (0-4 px) et pulsent sur les coups. Les macros ne sont que des chiffres. Vumètres de 18 px qui saturent vite | Fleurs proportionnelles au vrai niveau. Montrer l'effet des macros : Density = nombre de coups par piste, Motion = notes qui dévient, Colour = teinte et brillance, Space = halo |
| **SOUND** | ❌ Spirale de 44 points qui tourne sans rapport avec Tone, Harmonics ou Shape | **Mini-oscilloscope** de la voix (un bloc de 64 échantillons copié par image) ou, pour l'additif, ses harmoniques en barres. Tourner Tone ou Harm doit changer la forme |
| **RHYTHM** | ✅ Anneau euclidien + tête de lecture | Montrer la **probabilité** (coups pâles ou pleins selon Chance), les coups **réellement joués ou sautés**, les ratchets en petites subdivisions, l'accent sur le premier pas |
| **GRANULAR BUS** | ❌ 43 points décoratifs | Voir 15d (bande, tête d'écriture, grains, freeze) |
| **MIX / NOTES** | ❌ Texte seul | Pan en position gauche/droite, note de base sur un mini-clavier, division d'horloge en pulsations |
| **QUALITY / SYSTEM** | ✅ Chiffres CPU, xruns, mémoire (suffisant) | Ajouter un **historique CPU** en petite courbe et le passage ECO/FULL visible |
| **ENVELOPE / BLOOM** | ⚠️ Courbe **fausse** : la phase de decay ignore le réglage Decay (`app.c:312`), l'attaque est linéaire alors que le moteur est quadratique (`8·a²` ms) | Courbe exacte avec une échelle de temps, et un **point qui suit l'enveloppe réelle** de la voix (`v.env`) à chaque note |
| **GROOVE / REPEATS** | ❌ Texte seul | Grille de pas où le swing décale visiblement les contretemps, ratchets en sous-traits, hauteur des barres = accent |
| **TAPE / MOTION** | ❌ Texte seul (sens de lecture en flèche texte) | Bande avec sens et pitch des grains, et **vumètre de réduction du ducking** (`duckenv` existe déjà) |
| **HARMONY / GLOBAL** | ⚠️ Grosse lettre de tonalité + nom de la gamme | **Cercle ou clavier des 12 notes** : notes de la gamme allumées, **notes jouées en ce moment** qui flashent (`stats.last_note`), transposition visible. Indispensable pour les gammes exotiques et microtonales |
| **VELVET FILTER** | ⚠️ La courbe ne suit que le cutoff : résonance, pente (12/24), LPG et OFF sont ignorés, et la modulation Breathe/Tides n'est pas visible | Vraie réponse en fréquence avec la **bosse de résonance**, la pente selon le mode (et les futurs HP/BP/Notch), plus un **marqueur du cutoff réel qui bouge** avec les LFO |
| **LOOK / STARTUP** | Décoratif (normal : page d'apparence) | — |
| **GLOBAL MIXER** | ⚠️ Voir 15e | Voir 15e |
| **BASS / MOTIFS** | ✅ 16 points selon les notes | Ajouter la **tête de lecture**, le glide en liaison entre les notes, les notes variées par Marbles en surbrillance |
| **SHARED SPACE** | ❌ Texte seul | **Échos dessinés** : barres espacées selon le temps de delay, qui décroissent selon le feedback ; courbe de queue de la room ; niveau de retour ; plus tard, la grille de division du tempo |
| **MARBLES / TIDES / WARPS** | ❌ Texte seul | Triangle Tides avec **point de phase qui avance** (`tide_phase`) ; pas qui varient avec Marbles en surbrillance ; mélange Warps en jauge |
| **PHRASE / NOTES** | ⚠️ Grille de 16 nombres, curseur d'édition | **Mini piano-roll** (hauteur, silences, accent, chance), avec la tête de lecture |
| **MIDI / PERFORMANCE** | ❌ Texte seul | **Témoins d'activité MIDI** par canal (entrée et, plus tard, sortie), BPM de l'horloge reçue, notes entrantes. Plus tard, les zones de routage (lot 1) en bandes sur un clavier |
| **USER BANKS** | Texte (normal) | — |

**Éléments communs**
- **Fond « organique »** (pollen et pousses qui défilent, `md_organic`) : décoratif sur toutes les pages. Le relier au tempo (les pousses avancent au BPM et pulsent sur les temps) ou le rendre plus discret.
- **En-tête** : en plus du point lecture/stop, un **compteur de temps et de mesure** qui avance.
- **Vumètres** : passer partout à une échelle en dB (aujourd'hui `peak*100` ou `*260`, qui saturent vite).
- **Retour visuel immédiat** : quand on tourne un potard, surligner l'élément graphique concerné (pas seulement le chiffre).

**Coût** : l'affichage tourne dans la boucle principale (environ 12 images/s), séparé de l'audio. La plupart des données existent déjà (`md_status()`, `track_peak`, `step`, `last_note`). Il faut juste exposer quelques valeurs internes de plus (`env`, `tide_phase`, `duckenv`, position des grains, un bloc de 64 échantillons par voix pour l'oscilloscope). Coût audio quasi nul, quelques Ko de RAM au maximum.

**Fait (lot UI, plusieurs branches)** : 16 pages refaites d'après cet audit (tout le tableau sauf LOOK et USER BANKS, laissées en texte), vumètres en dB partout. Captures avant / après dans `docs/monades/captures/`.

**C'est un lot idéal pour commencer** : il n'ajoute aucune fonction risquée, il améliore tout de suite la compréhension, et il permet d'apprendre le code (UI + moteur) avant les gros chantiers.

### Lot Identité — Interface spatiale
Se démarquer visuellement de Mutable Dreams : remplacer son univers végétal (feuilles, fougères, pollen, « fleurs » de la page Live, cartes en forme de feuille) par un univers spatial (étoiles, galaxie, vaisseaux).
- Garder la règle de design : **rien de décoratif sans information**. Par exemple : les six voix en six vaisseaux sur la page Live, des étoiles qui s'allument sur les coups joués, un fond qui avance au tempo.
- Nouvelle palette et nouvel écran de démarrage, nom MONADES.
- Remplacer les cartes « feuille » des réglages K1–K4 par un style propre à Monades.
- Le site web passera aussi à l'interface spatiale, en temps utile.
- Pas tout de suite : après le lot UI, une fois les écrans fonctionnels validés sur le FM-1.
**Fait (branche `monades/identite`)** :
- palette « espace profond » et six couleurs d'étoiles, une par vaisseau ;
- fond étoilé : il défile à la vitesse du tempo pendant la lecture, s'arrête à l'arrêt ou en Motion STILL ; une traînée traverse la ligne d'une piste à chaque note. Fougères, pollen et tiges décoratives retirés (environ 14 Ko de mémoire libérés) ;
- page LIVE : la flotte des six vaisseaux sans fenêtres autour de son étoile (taille = niveau réel, flamme = hits après Density, tremblement d'orbite = Motion, éclat = Colour, anneau de poussière = Space, flamme blanche à chaque note, coque vide si muette) ;
- cadres K1–K4 en panneaux de coque à coins coupés, jauges droites graduées ;
- démarrage : MONADES, FLASH DOG DUKE SILVER, la flotte qui se met en formation ; FLASH DOG DUKE SILVER aussi sur la page SYSTEM ;
- site : espace profond par défaut (mode clair au choix), fond étoilé fixe, monades dessinées en vaisseaux colorés, nom Flash Dog Duke Silver au-dessus du titre. Aucun texte nouveau.
- petits éléments, chacun lié à une information : **fusée** à la place du voyant de lecture (moteur éteint à l'arrêt, flamme qui bat au tempo) ; **étoile filante** sous le titre à chaque changement de graine ; **ondes radio** pour les échos sur SHARED SPACE (rayon = temps, hauteur = feedback) ; **planètes** pour les quatre sections de FORM (taille = tension, anneau au sommet) ; **satellite** sur la page MIDI, qui émet une onde à chaque message envoyé.
**Tri (branche `monades/ui-sobre`)** : là où un élément porte une fonction, retour aux symboles habituels pour rester lisible à l'usage. ▶ / ■ pour la lecture (point sur chaque temps) à la place de la fusée ; ↻ avant le tempo pour une nouvelle variation à la place de l'étoile filante ; barres d'échos sur SHARED SPACE ; blocs de sections sur FORM ; plus de satellite sur MIDI (les voyants IN / OUT suffisent). On garde la flotte de LIVE, le fond étoilé, la palette et les panneaux.
**Reste** : valider les contrastes sur l'écran réel du FM-1.

### Lot Web — Site Monades
Le site de Mutable Dreams est remplacé par un site **plus simple, inspiré de la Monadologie** :
- une seule page, sobre et typographique (beaucoup de blanc, une seule couleur d'accent) ;
- structure en « propositions » numérotées, comme les 90 paragraphes de Leibniz (§1 Ce qu'est Monades, §2 Installer, §3 Les six monades, §4 Revenir au firmware d'origine…) ;
- l'instrument navigateur (build WASI) intégré en un bloc, sans surcharge visuelle ;
- liens : installation, retour V15, sources GitHub, crédits ;
- les visualisations suivent le même principe que le firmware : rien de décoratif.
**Fait** : le site est une vitrine (branche `monades/vitrine`) : le FM-1 jouable sur toute la largeur en haut (simulateur, voir plus bas), puis le mode d'emploi, le MIDI avec la MPC, l'installation (« bientôt »), le reste, la feuille de route repliée. Sous le simulateur, une section **Nouveautés** compare Mutable Dreams 1.1 et Monades ; le mode d'emploi donne la liste des 91 gammes, générée depuis `app/dsp/scale_table.inc` par `tools/site_scales.py` (branche `monades/nouveautes`).
**Reste** : la section Installer avec le vrai fichier quand il existera ; le MIDI depuis le simulateur.

### Après les lots
- **Revue du code** (tout ce qui a changé depuis Mutable Dreams) : 10 points, tous corrigés (branche `monades/revue`) : Stop MIDI au changement d'horloge, effacement du motif en une commande, notes de motif tapées ensemble, Biome qui vide les générateurs, pitch bend après le relâchement, passe-haut des envois recalculé seulement au changement, chorus isolé dans `chorus.cpp`, taille des patchs lue sur l'appareil.
- **Simulateur** (branche `monades/simulateur`) : Monades compilé pour le navigateur (WASI SDK 34) dans le panneau dessiné de fm1-base (`app/web/emu/index.html`, `tools/package_emu.py`). Publié sur le site.
- **Boutons ENV, LFO, ARP, REC** : chaque appui affiche ce qu'il a fait ; le simulateur indique la fonction sous ces boutons (UNDO, FREEZE, LOCK, DICE, MUTE).
- **Mondes et DICE** (branche `monades/monde-dice`) :
  - les Biomes s'appellent des **Mondes** à l'écran : « MONDE 12 DRONE », numéro et famille. Les noms viendront plus tard (catalogue `presets/catalog.json`). Le format des patchs ne change pas ;
  - **DICE sur REC** : sur EDIT, le son de la piste choisie ; sur SEQ, son rythme ; sur FX, les effets. Valeurs tirées dans des plages musicales (filtre jamais fermé, au moins un coup, retours d'écho et de grain loin du larsen). Jamais touchés : gamme, note de base, transposition, tempo, niveau, mutes, moteur, MIDI. ENV annule. Le dé a son propre générateur : la séquence ne change pas. Sur HOME et GLO, REC reste la nouvelle variation ;
  - l'aide de HARMONY indique 91 gammes au lieu de huit.
- **GLOBAL MIXER** (branche `monades/mixer`) : le vumètre M montre le niveau après MASTER (avant, il ignorait le potard, défaut hérité de Mutable Dreams) ; les cadres K1 à K4 affichent la piste réglée (« K1 LEVEL T6 »).
- **Écrans et aides** (branche `monades/ecrans`), après une revue des 27 pages :
  - numéro de la piste réglée après le titre sur BASS / MOTIFS, PHRASE, GENERATOR, MARBLES / TIDES / WARPS ;
  - messages raccourcis qui sortaient de l'écran ou étaient recouverts (MIDI OUT ZONES, FORM) ;
  - aides mises à jour : cycles SEQ, FX et GLO complets, titre GLOBAL au lieu de SYSTEM, HARMONY mène à Raga, 8 modes de filtre, aide propre à MIDI OUT ZONES ;
  - DICE des effets toujours audible : un retour d'effets à 0 remonte, et sans aucun envoi la piste choisie reçoit de l'écho et de la room ;
  - test `help_library` réparé (il comptait les pages GLO d'avant la page RAGA).
- **Mondes FDDS** (branche `monades/mondes-fdds`) : 14 Mondes numéros 78 à 91, un par lieu ou personnage du wiki FDDS (MACHIN, GADIN, MARTINI, CHRISTELE, SARL, CITRONS, CARGO, VEGA, FENOUIL, CLOUDEE, HENDIATRIQUE, ALCHIMISTE, COGITO, JARDINIERS). Ce sont des patchs v10 complets : LFO, générateurs, forme, gamakas, notes pivots, gammes microtonales, chorus, écho synchronisé. Écrits dans `tools/fdds_mondes.py`, générés par `tools/generate_presets.py` ; les 77 Mondes de Mutable Dreams sont inchangés et se chargent toujours avec les nouveautés coupées. L'écran affiche leur nom (« MONDE 81 CHRISTELE »). Les pistes clairsemées ont Chance à 100 % : avec la graine verrouillée, Chance se répète à chaque tour et une piste à deux coups pouvait ne jamais jouer.
- **Morphing** (branche `monades/morphing`) : page MORPH (HOME deux fois). Le patch en cours est A, K1 ou PRESET choisit le Monde B, K2 la position, K3 un passage automatique sur 4 à 64 mesures pendant la lecture, K4 ce qui change (tout, son, rythme, effets). Chaque mot du patch a un type (`app/dsp/morph.inc`) : les nombres glissent, les choix basculent à mi-chemin, le reste (mutes, verrou, qualité) ne bouge pas. REC garde le mélange, ENV revient à A. L'écran montre A qui s'éteint, B qui s'allume et la flotte du son en cours qui va de l'un à l'autre, le tempo, les deux gammes et les moteurs qui ont basculé. Mémoire : deux patchs de plus (3,4 Ko).
- **Fondu du morphing** (branche `monades/morph-fondu`) : les choix de chaque piste basculent à leur propre moment (17, 30, 43, 57, 70, 83 %) au lieu de tous à 50 % ; autour de ce moment, la piste descend jusqu'au silence puis remonte (±6 %), donc la voix redémarre sans qu'on l'entende. Gamme, note de base et graine basculent à 50 %. REC garde la piste à son plein niveau. Sur un morphing de 8 mesures MACHIN → CHRISTELE, le plus grand saut de niveau sur 20 ms passe de 9,6 à 6,9 dB (mesure sur le Mac, coups de batterie compris).
- **DERIVE** (branche `monades/derive`) : page DERIVE (HOME trois fois). Le Monde glisse tout seul, avec le moteur du morphing, vers des variantes tirées autour de sa maison (le patch au départ), une après l'autre, pendant la lecture (`app/dsp/drift.inc`). K1 distance (près, moyen, loin), K2 durée d'un glissement (OFF, 8 à 256 mesures), K3 voyage (rester, la famille, n'importe quel Monde ; un glissement sur quatre), K4 ce qui dérive. Pistes qui se taisent un glissement, nouvelles notes, tempo ±10 %. Gamme, note de base, transposition et mutes ne bougent jamais. REC nouvelle maison, ENV retour à la maison. Écran : maison au centre, flotte en orbite selon la distance, temps écoulé, prochain glissement, trois derniers changements. Testé 12 minutes simulées sur le Mac ; à vérifier sur des heures et sur le FM-1. Mémoire : un patch de plus (1,7 Ko).

### Lot 5 — Divers
17. **Sortie audio USB** (aujourd'hui `usb_audio_out = false`, entrée seulement), si le budget CPU le permet.
18. Option : un K2 Motion à vitesse liée au tempo (variation tous les N pas plutôt qu'à chaque pas).
19. Refactorisation progressive de `dreams.cpp` (580 lignes très denses) en modules : séquenceur, gammes, MIDI, effets.
   **Fait (branche `monades/divers`)** :
   - 17 : l'enregistrement du FM-1 par USB fonctionne déjà ; la sortie vers le FM-1 reste coupée, documentée dans le README, à essayer sur l'appareil ;
   - 18 : **K4 MOTION** sur la page FORM. La variation de Motion change tous les 1, 2, 4, 8 ou 16 pas et tient entre deux. Patch v10 (1680 octets) ;
   - 19 : les parties Monades du moteur sortent de `dreams.cpp` : `midi_out.inc`, `lfo.inc`, `generative.inc`. Même unité de compilation, son identique au bit près. Le cœur Mutable Dreams (voix, séquenceur, effets) reste en place.

## Prochaines étapes
1. **Essayer dans le simulateur** que les fonctions répondent aux besoins.
2. **Compiler pour le FM-1** (chaîne JieLi pi32v2 + SDK AC79, voir `BUILDING.md`) : la compilation dira si la mémoire tient.
3. **Tester sur le FM-1** en suivant `docs/monades/TESTS-FM1.md` (charge du processeur d'abord). Essayer aussi le V15 d'origine, Felucca et Mutable Dreams.
4. **Publier une version** : la section Installer du site, et peut-être The Firmware Hub (`FIRMWARE-HUB.json` hérité de Mutable Dreams).
Chaque changement se développe d'abord sur le Mac (`tools/build_local.py host`), puis dans le navigateur, puis sur le FM-1.
