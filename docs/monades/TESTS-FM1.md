# Monades 0.1 : tests sur le FM-1

À dérouler dans l'ordre le jour où le FM-1 est là. Cocher, et noter les chiffres demandés : ce sont eux qui diront si le firmware tient.

Rien de tout cela n'a encore été fait : le code n'a tourné que sur Mac (simulateur et tests automatiques).

## 0. Avant de commencer
- [ ] Le FM-1 démarre sur son firmware d'origine (V15, identité `FM-1_015`). Noter sa version.
- [ ] Le fichier d'origine `FM-1-V15.fwsc` est récupéré et son empreinte vérifiée (taille 699956 octets, SHA-256 dans `Recovery-1.1.txt`). C'est le retour possible à tout moment.
- [ ] Optionnel : essayer d'abord Mutable Dreams 1.1, pour avoir un point de comparaison (son, charge processeur). Si des biomes y sont sauvés, faire une sauvegarde `.mddb` avant de passer à Monades (`Recovery-1.1.txt`).
- [ ] Un câble USB de données, Chrome ou Edge, Python 3.11 pour l'installeur en ligne de commande.

## 1. Compiler
- [ ] Chaîne JieLi pi32v2 et SDK AC79NN V1.2.1 installés (voir `BUILDING.md`, section « FM-1 device build »).
- [ ] `sh tools/build_device.sh` produit un `.fwsc` sous `app/build/`.
- [ ] **Noter la mémoire affichée** par la compilation : « RAM .data+.bss … of 98304 » et « pool … of 344064 ». Mutable Dreams 1.1 : 71768 et 334800. Si l'un des deux dépasse, la compilation échoue : me donner les chiffres.

## 2. Installer
- [ ] `python3 base/tools/fm1_install.py --info` voit le FM-1.
- [ ] `python3 base/tools/fm1_install.py Monades-0.1.0.fwsc --yes` (sans `--force` : le paquet porte le marqueur du chargeur). Garder l'USB branché jusqu'à la confirmation.
- [ ] Le FM-1 redémarre avec l'identité **`FM-1_714000100`** et le port MIDI USB **« Monades FM-1 »**.
- [ ] Écran de démarrage : MONADES, FLASH DOG DUKE SILVER, la flotte qui se met en formation, carillon si START SOUND est sur ON.

## 3. Charge du processeur (le plus important)
Lire la page **SYSTEM** (GLO, trois fois) : LOAD / MAX % et XRUNS.
- [ ] À l'arrêt : LOAD = … %
- [ ] Biome par défaut en lecture : LOAD = … %, MAX = … %
- [ ] Biome dense (six pistes, Density haute) : MAX = … %, XRUNS = …
- [ ] Le même avec, en plus, CHORUS MIX 60, DIFFUSION 70, SEND HIGH-PASS sur ON (pages ECHO SYNC / CHORUS et SPACE TONE) : MAX = … %, XRUNS = …
- [ ] Deux LFO actifs sur chaque piste : MAX = … %
- [ ] Le message « ECO: DENSE PATCH » apparaît-il ? Quand ?
- Attendu : pas de craquement, XRUNS qui ne monte pas en jeu normal. Mutable Dreams montait vers 85 %.

## 4. Écran
- [ ] Les textes se lisent sur l'écran réel (palette sombre, petits textes gris).
- [ ] Page LIVE : les six vaisseaux se distinguent, la flamme blanche part à chaque note, la piste choisie a son anneau.
- [ ] La fusée en haut à droite flambe au tempo pendant la lecture.
- [ ] REC : l'étoile filante passe sous le titre.
- [ ] Les panneaux K1–K4 et leurs jauges se lisent.
- [ ] LOOK : MOTION sur STILL arrête toutes les animations ; les informations restent.
- Noter les écrans peu lisibles : …

## 5. MIDI avec la MPC
- [ ] Notes reçues : la MPC joue le FM-1 sur les canaux 1 à 6 (comme Mutable Dreams).
- [ ] MIDI / PERFORMANCE, K3 NOTES OUT = TRACKS 1-6 : la MPC reçoit chaque piste sur son canal.
- [ ] K3 = ZONES, avec les zones par défaut (C1–B2 canal 1, C3–B4 canal 2, C5–C7 canal 3) : chaque note arrive sur le canal de sa zone ; zones qui se chevauchent = note sur les deux canaux.
- [ ] K4 CLOCK OUT : la MPC suit le tempo, démarre et s'arrête avec PLAY.
- [ ] Gamme microtonale (Rast) en TRACKS : la MPC reçoit le pitch bend (régler sa plage sur ±2 demi-tons) ; en ZONES : notes arrondies, pas de bend.
- [ ] À l'arrêt et en PANIC : aucune note ne reste bloquée sur la MPC.
- [ ] Page MIDI : le satellite émet une onde à chaque message envoyé.

## 6. Audio par USB
- [ ] L'ordinateur voit le FM-1 comme entrée audio et l'enregistre (44,1 kHz stéréo), sans trou.
- [ ] Optionnel, plus tard : compiler avec `usb_audio_out = true` et vérifier que l'ordinateur joue à travers le FM-1, en surveillant la page SYSTEM.

## 7. Sauvegardes
- [ ] SAVE à l'arrêt : « BIOME + PREFS SAVED ». Éteindre, rallumer : le biome et les préférences (MIDI out, zones, gamme CUSTOM) reviennent.
- [ ] GLO > USER BANKS : sauver un biome et un son, les recharger.
- [ ] Si une sauvegarde `.mddb` de Mutable Dreams existe : la restaurer (`python3 tools/dreams_backup.py restore … --port 'Monades FM-1'`, puis SEL sur le FM-1). Les biomes de Mutable Dreams doivent se charger, toutes les nouveautés au neutre.

## 8. Son
Pour chacun : ça sonne, sans clic ni saturation, et l'écran suit ce qu'on entend.
- [ ] Filtres : HP 12, HP 24, BP, NOTCH (EDIT > VELVET FILTER).
- [ ] LFO A / B : une division synchronisée sur le cutoff, le sample & hold sur NOTES IN SCALE.
- [ ] Écho synchronisé (1/8 à 120 BPM = 250 ms), chorus, diffusion de la room.
- [ ] Générateur : RANDOM, MARKOV, MOTIF (motif tapé au clavier), DEJA VU à 0, 85 et 100.
- [ ] FORM : calme, montée, sommet, détente ; des pistes se reposent dans le calme.
- [ ] RAGA : vadi et samvadi, gamakas KAN, MEEND, ANDOLAN.
- [ ] Gammes : un raga, une gamme microtonale, la gamme CUSTOM jouée au clavier.

## 9. Revenir en arrière si besoin
- [ ] Mutable Dreams : `python3 base/tools/fm1_install.py Mutable-Dreams-1.1.0.fwsc --yes`.
- [ ] Firmware d'origine : `python3 base/tools/fm1_install.py --force --yes FM-1-V15.fwsc` (le seul cas où `--force` sert). V15 efface les banques utilisateur : garder la sauvegarde `.mddb`.

## Ce qu'il faut me rapporter
Les chiffres des sections 1 et 3, les écrans peu lisibles, et tout ce qui coince (son, MIDI, sauvegardes), avec la page et les réglages.
