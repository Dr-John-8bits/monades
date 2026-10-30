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
- **Mode d'emploi complet** (branche `monades/manuel`) : page `site/manuel/` avec recherche (filtre et surlignage, accents ignorés, `?q=` dans l'adresse, touche `/`), sommaire, « Où trouver quoi » en tête (le bouton LFO fige le grain, les LFO sont sous FX), les 14 boutons, les encodeurs, les pages avec K1 à K4, des marches à suivre (zones MIDI, sauvegarde, MPC…), le MIDI, les listes. Gammes et Mondes FDDS générés par `tools/site_scales.py` et `tools/fdds_mondes.py` comme sur l'accueil.
- **Lumières des boutons** (branche `monades/lumieres`) : le bouton de la page affichée (HOME, EDIT, SEQ, FX ou GLO) est allumé, au lieu de HOME en permanence (Mutable Dreams) ; SEL s'allume quand la piste choisie est coupée ; ENV luit faiblement quand il y a quelque chose à annuler ; ARP, LFO et PLAY comme avant. Le simulateur affiche la lueur faible. Test `tests/lights_ui.txt`.
- **Revue 2** (branche `monades/revue-2`), avant le premier flash : un réglage tourné pendant un morphing automatique ou une dérive reste (il est écrit dans A et B, et dans la maison pendant la dérive) ; DICE et REC pendant la dérive durent le glissement, ENV ramène toujours à la maison ; charger un Monde remet K2 de DERIVE sur OFF ; une piste à 0 coup reste muette ; sur MORPH, K2 à K4 et REC ne touchent plus la dérive ; graine libre, le morphing ne la remet plus en place ; « quelque chose à annuler » est calculé par le moteur après chaque commande et chaque pas (plus de comparaison à chaque image) ; les cycles de pages et les lumières lisent la même table. Mémoire estimée avec le compilateur du Mac : le moteur demande environ 20 Ko de plus que Mutable Dreams 1.1, dont 4 Ko dans le pool (chorus) et 16 Ko en RAM (patchs plus grands, diffusion, morphing et dérive : quatre patchs de 1,7 Ko). Soit environ 88 Ko de RAM sur 98 et 339 Ko de pool sur 344. À confirmer par la compilation pour le FM-1.
- **DICE sur ce qu'on voit** (branche `monades/dice-page`) : REC tire au sort les réglages K1 à K4 de la page affichée (19 pages : SOUND, FILTER, ENVELOPE, MIX, MIXER, RHYTHM, GROOVE, BASS, PHRASE, FORM, GRANULAR, TAPE, SPACE, SPACE TONE, ECHO / CHORUS, MARBLES, LFO A, LFO B, RAGA), pour la piste choisie quand c'est une page de piste. Avant : un DICE par groupe (son sur EDIT, rythme sur SEQ, effets sur FX), qui ne touchait pas forcément la page affichée (sur LFO A, il changeait les effets). Plages musicales par réglage ; le niveau ne change que sur MIXER.
- **Pile du fil audio** (branche `monades/pile`) : charger un Monde, choisir le Monde B et préparer un glissement de dérive écrivaient chacun un `md_patch` entier (1 680 octets) sur la pile du fil audio, jusqu'à environ 5 Ko dans `tick`. Ils écrivent maintenant directement dans `patch` et `morph_b`. Mesure `-fstack-usage` (clang, -O2) : `tick` 5 248 → 336 octets ; chemin le plus profond `md_render` → `quantum` → une voix ≈ 3,7 Ko, contre ≈ 3,4 Ko pour Mutable Dreams 1.1.
- **Revue externe** (branche `monades/revue-externe-corrections`), faite par une autre session de Claude :
  - bloquant : le pool doit garder 8 192 octets libres (`base/tools/build.py`), soit 335 872 au plus ; avec le chorus il en restait environ 5 100 et la compilation pour le FM-1 aurait refusé de produire le paquet. Le chorus passe en RAM ordinaire (pool estimé à 334 900) ;
  - charger un emplacement USER (ou un patch de l'éditeur) termine le morphing et la dérive ;
  - surcharge du calcul : une commande `MD_OVERLOAD` remplace `MD_QUALITY 0` ; en FULL elle passe en ECO, déjà en ECO elle coupe chorus et diffusion jusqu'au prochain Monde ; elle ne marque plus le Monde comme modifié. Le seuil d'admission en FULL reste 85 : il reproduit des limites mesurées sur le FM-1 en 1.1 (4 FM restent en FULL, 5 passent en ECO, test `dsp-tests`) ; la revue estime les voix de Monades environ 8 % plus lourdes en FULL : à mesurer sur l'appareil (page SYSTEM, 6 VA + écho, room et chorus), le passage en ECO sur surcharge reste le filet ;
  - MIDI : les messages qui ne tiennent pas dans la file USB restent dans celle du moteur (aucune fin de note jetée), vidée à chaque nouvel hôte ; PANIC envoie CC 123 ; horloge sortante basculée pendant la lecture : start ou stop ; la même source d'horloge n'arrête plus la lecture ;
  - bornes : note MIDI d'essai bornée, piste LFO hors plage ignorée ; une seule lecture des valeurs du moteur avant de les utiliser comme indice ; dérive des étoiles bornée ; oscilloscope enregistré pour la seule piste affichée ;
  - vérifications : les 77 Mondes de Mutable Dreams donnent un son identique au bit près à Mutable Dreams 1.1 (12 s chacun) ; les tables régénérées sont identiques aux fichiers enregistrés ; morphing sur les 91 × 91 paires (et les quatre portées pour une partie) : 201 110 états, tous valides ;
  - laissés de côté : les points hérités de la plateforme fm1-base (silence audio pendant l'effacement flash, démarrage raté compté pendant une restauration USB, garde de pile matérielle) et le message de restauration USB, hérité de 1.1 ; une piste au niveau 0 envoie toujours ses notes MIDI (elle peut servir de piste MIDI seule) ; `FIRMWARE-HUB.json` décrit encore Mutable Dreams, à mettre à jour avant une publication.
- **Canaux MIDI par piste** (branche `monades/canaux-midi`) : nouvelle page GLO › MIDI CHANNELS (entre MIDI et MIDI OUT ZONES). Pour chaque piste, le canal écouté (1 à 16 ou OFF) et le canal de sortie en TRACKS (1 à 16 ou OFF) ; K4 ALL FROM met les six pistes à la suite (sur 3 : canaux 3 à 8, pour laisser 1 et 2 à d'autres machines). Par défaut la piste n sur le canal n, comme avant. Plusieurs pistes peuvent écouter le même canal ; le canal 16 garde les macros sur les CC 16 à 19. Changer le canal d'entrée d'une piste relâche la note tenue ; changer une sortie relâche les notes en cours. PANIC envoie CC 123 sur les canaux de sortie utilisés. Les canaux sont ajoutés à la fin des préférences : les préférences sauvées avant (et les sauvegardes USB) se lisent toujours. Les points IN et OUT de la page s'allument avec les notes reçues et envoyées par piste.
- **Première compilation pour le FM-1** (branche `monades/compil-fm1`, 28 octobre 2026) : `monades-0.1.0.fwsc`, 610 136 octets, identité `FM-1_714000100`, marqueur du chargeur Felucca présent. Image 443 556 octets ; RAM 80 584 sur 98 304 ; pool 334 872 sur 344 064. La première tentative s'arrêtait à l'édition des liens : `monde_label` (écran) appelait `strcmp`, qui n'existe pas sur l'appareil ; remplacé par une comparaison écrite à la main. Tous les autres appels de bibliothèque sont sous `PLAT_HOST`. Le Bluetooth MIDI n'est pas compris (`ble = false`, comme Mutable Dreams) : mode d'emploi corrigé. Environnement sur Mac Apple Silicon, dans `~/.jieli` : chaîne `jieli-linux-toolchains-20260730.1` (serveur JieLi) ; SDK AC79NN V1.2.1 à la révision `d179b44` via `SDK_URL=https://github.com/amitv87/fw-AC79_AIoT_SDK` (le dépôt officiel Jieli-Tech n'existe plus ; empreintes vérifiées par `get_sdk.sh`) ; Blink `f006a4f` avec `tools/toolchain-patches/blink-macos-guest-pages.patch`, compilé avec GNU make 4.4.1 ; bibliothèques Linux de Debian 12 (libc6, libstdc++6, libgcc-s1, empreintes du dépôt Debian) dans `~/.jieli/sysroot`, avec `/lib64/ld-linux-x86-64.so.2` copié (pas en lien absolu). Variables : `JIELI_RUNNER="~/.jieli/blink-src/o/blink/blink -m -j"`, `BLINK_OVERLAYS="~/.jieli/sysroot:"`, `JIELI_TOOLCHAIN=~/.jieli/toolchain`, `AC79_SDK=~/.jieli/ac79-sdk`, `ulimit -n 8192`. Durée : environ 25 minutes. Les tests de `base/` passent (installeur, sauvegarde, stockage).
- **Site en plusieurs pages** (branche `monades/site-simple`) : accueil (le synthé, le simulateur, l'installation, les crédits), `nouveautes/` (le tableau des nouveautés, les Mondes et les Mondes FDDS, le morphing, la dérive, les gammes), `manuel/` (plus un aide-mémoire : les pages par bouton, les autres commandes, K1 à K4 page par page), `midi/` (la MPC), `feuille-de-route/` (le principe, les lots, les prochaines étapes). Un menu commun (`tools/site_nav.py`), replié derrière un bouton sur téléphone ; feuille de style et script communs (`site/style.css`, `site/menu.js`). Les anciens liens (`#mpc`, `#lots`, `#gammes`…) renvoient vers la nouvelle page. `tools/site_scales.py` et `tools/fdds_mondes.py` écrivent maintenant dans `site/nouveautes/index.html`. Plus aucune mention de Biome sur le site ; l'image du FM-1 éteint (`site/ecrans/live.png`) refaite avec l'écran actuel (LIVE MONDE).
- **Installeur dans le navigateur** : `tools/package_monades_installer.py` fait `site/installer/` à partir de l'installeur de fm1-base (Felucca), en français avec l'anglais à un clic, le paquet dans la page et vérifié par son SHA-256, l'identité contrôlée après le redémarrage (comme Mutable Dreams 1.1). Il copie aussi `monades-0.1.0.fwsc` et `fm1_install.py` pour l'installation depuis le Mac. Publié avant tout test sur un FM-1, à la demande, avec l'avertissement sur le site et dans l'installeur.
- **Sauvegarde `.mddb`** : `tools/dreams_backup.py` n'acceptait que l'identité de Mutable Dreams (`FM-1_710`) ; il accepte aussi Monades (`FM-1_714`, mêmes objets) et cherche le port « FM-1 ».
- **Liste des 91 Mondes** dans le mode d'emploi (`tools/site_mondes.py`, depuis `presets/catalog.json`) : numéro et famille comme à l'écran (nom pour les FDDS), gamme, note, tempo, moteurs des six pistes ; plus « Où garder ses propres Mondes » (SAVE, 16 emplacements de Mondes, 64 de sons, sauvegarde `.mddb`).
- **Premier essai sur un FM-1** (29 octobre 2026) : installé depuis la V15 (`FM-1_015`) avec `base/tools/fm1_install.py --yes`, identité `FM-1_714000100` et port « Monades FM-1 » après le redémarrage ; il démarre et joue. Le lien officiel de M-VAVE donne depuis le 21 octobre 2026 un fichier de 718 196 octets (pas la V15 de 699 956 octets dont l'empreinte est attendue par l'installeur et `fm1_install.py`) : retour au firmware de M-VAVE par M-UPGRADE dans ce cas.
- **Version 0.1.1** (29 octobre 2026) : audio USB. Sur le FM-1, l'enregistrement USB craquait sur les Mondes chargés (rendu à 70-80 % du temps), propre sur un Monde léger : la réserve par défaut de `felucca-bsp/chip/usb.c` ne tient que si un rendu prend moins de ~40 % de son temps. `FELUCCA_UAC_DEEP 1` dans `app/app.h` (réserve de 1 024 trames, +5,8 ms, +2 Ko de RAM : 82 632 sur 98 304) : enregistrement propre, essayé sur le FM-1. Reste un crachotement au changement de Monde (PRESET) : `scene()` efface la bande (`clear_audio`) et remet les six voix à zéro dans un seul bloc, probablement plus long qu'un bloc. Tempo sur horloge reçue : les tops sont lus au début de chaque bloc (5,8 ms), d'où ±1 à 2 BPM affichés.
- **Mode d'emploi : les effets** : une partie « Les effets : envois et retours » avec deux schémas (le trajet du son : piste, envois, passe-haut, grain, écho, room, retours SPACE et RETURN, chorus sur le son à sec, MASTER ; le ducking dans le temps), les effets un par un, le ducking en détail, des recettes et « je n'entends pas l'effet ». La page MARBLES / TIDES / WARPS explique exactement K1 à K4 (d'après le moteur).
- **LFO sur la rotation** (branche `monades/lfo-rotate`) : 20e cible `MD_TO_ROTATE` (ajoutée à la fin : les patchs sauvés ne changent pas) ; le motif euclidien tourne de `lfo × longueur` pas autour de K3 (profondeur 100 = un tour). Calculé une fois par pas et par piste (un modulo), pas par échantillon : coût négligeable. `stats.hits_now` et `stats.rotate_now` : l'anneau de RHYTHM et la grille de GROOVE montrent en lecture les coups et la rotation vraiment joués (Density, FORM et LFO compris). Page LFO : « NOW +n STEPS », et « STEP MODE: EVERY STEP PLAYS » si la piste est en STEP.

### Lot 5 — Divers
17. **Sortie audio USB** (aujourd'hui `usb_audio_out = false`, entrée seulement), si le budget CPU le permet.
18. Option : un K2 Motion à vitesse liée au tempo (variation tous les N pas plutôt qu'à chaque pas).
19. Refactorisation progressive de `dreams.cpp` (580 lignes très denses) en modules : séquenceur, gammes, MIDI, effets.
   **Fait (branche `monades/divers`)** :
   - 17 : l'enregistrement du FM-1 par USB fonctionne déjà ; la sortie vers le FM-1 reste coupée, documentée dans le README, à essayer sur l'appareil ;
   - 18 : **K4 MOTION** sur la page FORM. La variation de Motion change tous les 1, 2, 4, 8 ou 16 pas et tient entre deux. Patch v10 (1680 octets) ;
   - 19 : les parties Monades du moteur sortent de `dreams.cpp` : `midi_out.inc`, `lfo.inc`, `generative.inc`. Même unité de compilation, son identique au bit près. Le cœur Mutable Dreams (voix, séquenceur, effets) reste en place.
20. **Gamme par piste** (plus tard). Aujourd'hui la gamme est un réglage du Monde, la même pour les six pistes, et il n'y a qu'une gamme CUSTOM pour toute la machine (12 demi-tons, notes ajoutées ou retirées au clavier sur HARMONY, gardée dans les préférences). À faire : une gamme par piste (et peut-être une gamme CUSTOM par piste). Compléter aussi le mode d'emploi sur CUSTOM : commune à tous les Mondes, 12 demi-tons seulement, les six pistes ; les gammes Scala (`.scl` dans `tools/scala/`) ne s'ajoutent qu'à la compilation.

21. **Clavier** (plus tard) : les fonctions imprimées au-dessus des touches noires (OP1, OP2, OP3, OP4, OP5, OP6, PIT, GLO, MONO, POLY ; dans l'ordre F#, G#, A#, C#, D#, F#, G#, A#, C#, D#) ne sont pas utilisées, ces touches jouent leur note ; les lumières des touches s'allument seulement sous le doigt et pourraient montrer une information (gamme, notes jouées, pas) ; OCT− et OCT+ ne font que décaler l'octave du clavier.
22. **Deux FM-1 en duo** (plus tard). Sans MPC ni ordinateur, deux FM-1 ne se parlent pas par câble (appareils USB, prise TRS en entrée seulement). fm1-base sait faire du Bluetooth MIDI (`ble = true`) et se connecter à un autre appareil Bluetooth (`ble_central = true`) : à essayer dans Monades (RAM aujourd'hui 82 632 sur 98 304 ; charge du processeur à mesurer). Puis un mode duo : envoyer la gamme, la note de base, les changements de Monde et les macros (CC 16 à 19 sur le canal 16, que Monades reçoit déjà), option « inverse » sur les macros, question / réponse par le générateur MARKOV. Pour le deuxième FM-1, d'après The Firmware Hub (29 octobre 2026) : Hortator (DEADACTIVE, GPL-3.0 : 8 pistes de batterie, Grids, horloge MIDI en entrée par USB ou TRS, version Bluetooth qui reçoit une horloge sans fil, audio USB sur 10 canaux) ou X0X (Charles Vestal, GPL-3.0 : 909, 808, deux 303 avec TB-3PO, breakbeats) ; CTL-1 (Charles Vestal, MPL-2.0) comme contrôleur MIDI qui émet en Bluetooth.
23. **Pistes séparées en audio USB** (plus tard) : Hortator le fait (8 pistes, 10 canaux) ; Monades envoie le mélange stéréo seulement.
24. **Mutable Dreams 1.2** (plus tard) : 15 moteurs, 288 sons, 92 Biomes, enchaînements d'accords ; voir ce qui vaut la peine d'être repris dans Monades.
25. **Deuxième FM-1 : un synthé granulaire** (une piste parmi d'autres, avec le firmware existant pour le rythme et Monades en duo). Grains, réverbe, delay ping-pong, tape, shimmer, égaliseur, compresseur, écrans qui montrent les grains. Sources : audio USB, échantillons du navigateur, oscillateurs internes (pas d'entrée audio sur le FM-1). Mémoire : RAM 98 Ko, zone de 336 Ko, flash de 1 Mo. Deuxième cœur : code présent dans `base/bsp/docs/EXPERIMENTAL.md`, jamais exécuté. Référence d'idées seulement : GrainBrick (TrimUI Brick, code fermé, non repris).

## Prochaines étapes
1. **Essayer dans le simulateur** que les fonctions répondent aux besoins.
2. **Compiler pour le FM-1** (chaîne JieLi pi32v2 + SDK AC79, voir `BUILDING.md`) : la compilation dira si la mémoire tient.
3. **Tester sur le FM-1** en suivant `docs/monades/TESTS-FM1.md` (charge du processeur d'abord). Essayer aussi le V15 d'origine, Felucca et Mutable Dreams.
4. **Publier une version** : la section Installer du site, et peut-être The Firmware Hub (`FIRMWARE-HUB.json` hérité de Mutable Dreams).
Chaque changement se développe d'abord sur le Mac (`tools/build_local.py host`), puis dans le navigateur, puis sur le FM-1.
