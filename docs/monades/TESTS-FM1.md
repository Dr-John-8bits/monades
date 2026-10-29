# Monades 0.1 : tests sur le FM-1

À dérouler dans l'ordre le jour où le FM-1 est là. Cocher, et noter les chiffres demandés : ce sont eux qui diront si le firmware tient.

Rien de tout cela n'a encore été fait : le code n'a tourné que sur Mac (simulateur et tests automatiques).

## 0. Avant de commencer
- [ ] Le FM-1 démarre sur son firmware d'origine (V15, identité `FM-1_015`). Noter sa version.
- [ ] Le fichier d'origine `FM-1-V15.fwsc` est récupéré et son empreinte vérifiée (taille 699956 octets, SHA-256 dans `Recovery-1.1.txt`). C'est le retour possible à tout moment.
- [ ] Optionnel : essayer d'abord Mutable Dreams 1.1, pour avoir un point de comparaison (son, charge processeur). Si des Mondes y sont sauvés, faire une sauvegarde `.mddb` avant de passer à Monades (`Recovery-1.1.txt`).
- [ ] Un câble USB de données, Chrome ou Edge, Python 3.11 pour l'installeur en ligne de commande.

## 1. Compiler
- [x] Chaîne JieLi pi32v2 et SDK AC79NN V1.2.1 installés (voir `BUILDING.md`, section « FM-1 device build »). Fait le 28 octobre 2026 sur le Mac (Apple Silicon), voir `FORK.md`.
- [x] `sh tools/build_device.sh` produit un `.fwsc` sous `app/build/` : `monades-0.1.0.fwsc`, 610 136 octets, identité `FM-1_714000100`.
- [ ] **Noter la mémoire affichée** par la compilation : « RAM .data+.bss … of 98304 » et « pool … of 344064 ». Mutable Dreams 1.1 : 71768 et 334800. La compilation échoue si la RAM dépasse 98 304, ou si le pool laisse moins de 8 192 octets libres (donc au-delà de 335 872) : me donner les chiffres. Mesuré pour Monades le 28 octobre 2026 : image 443 556 octets, RAM 80 584 sur 98 304, pool 334 872 sur 344 064 (1 000 octets sous la limite de 335 872). Pas de calcul en double précision, accès aux registres par felucca-bsp seulement.

## Résultats du 29 octobre 2026 (FM-1 de l'auteur, depuis la V15)
- Installation 0.1.0 par `fm1_install.py --yes` : identité `FM-1_714000100` et port « Monades FM-1 » après le redémarrage ; il démarre et joue.
- Charge (SYSTEM, LOAD / MAX) : 89 ALCHIMISTE 76 / 77 %, 87 CLOUDEE 69 / 77 %, en ECO, XRUNS 0.
- Sortie casque propre. Audio USB : craquements et tremblements dans l'enregistrement sur les Mondes chargés en 0.1.0 (propre sur 65 BLANK joué au clavier) ; propre avec la réserve profonde (`FELUCCA_UAC_DEEP`, version 0.1.1). Reste : un crachotement dans l'enregistrement au changement de Monde avec PRESET.
- Horloge MIDI reçue par l'USB et par la prise TRS (Roland TR-6S, câble TRS-DIN Boss ; la prise MIDI OUT de la TR-6S) : le FM-1 suit ; le tempo affiché oscille de 109 à 112 pour 110 envoyés.

## 2. Installer
- Partir de la V15 (`FM-1_015`), pas de la V16 : des problèmes d'installation ont été signalés par des utilisateurs partis de la V16. Le lien officiel de M-VAVE donne depuis le 21 octobre 2026 un fichier plus récent (718 196 octets) que la V15 (699 956 octets).
- [ ] `python3 base/tools/fm1_install.py --info` voit le FM-1.
- [ ] `python3 base/tools/fm1_install.py Monades-0.1.0.fwsc --yes` (sans `--force` : le paquet porte le marqueur du chargeur). Garder l'USB branché jusqu'à la confirmation.
- [x] Le FM-1 redémarre avec l'identité **`FM-1_714000100`** et le port MIDI USB **« Monades FM-1 »**. Fait le 29 octobre 2026, depuis la V15 (`FM-1_015`), avec `fm1_install.py --yes` : écriture complète, identité vérifiée après le redémarrage ; il démarre et joue.
- [ ] Écran de démarrage : MONADES, FLASH DOG DUKE SILVER, la flotte qui se met en formation, carillon si START SOUND est sur ON.

## 3. Charge du processeur (le plus important)
Lire la page **SYSTEM** (GLO, trois fois) : LOAD / MAX % et XRUNS.
- [ ] À l'arrêt : LOAD = … %
- [ ] Monde par défaut en lecture : LOAD = … %, MAX = … %
- [ ] Monde dense (six pistes, Density haute) : MAX = … %, XRUNS = …
- [ ] Le même avec, en plus, CHORUS MIX 60, DIFFUSION 70, SEND HIGH-PASS sur ON (pages ECHO SYNC / CHORUS et SPACE TONE) : MAX = … %, XRUNS = …
- [ ] Deux LFO actifs sur chaque piste : MAX = … %
- [ ] Le Monde 89 ALCHIMISTE (le plus lourd des Mondes FDDS) : MAX = … %, XRUNS = …
- [ ] En FULL (SYSTEM, K2) : six pistes PLAITS VA, écho, room et chorus. MAX = … % : Monades reste-t-il en FULL, et sous 90 % ? Sinon, abaisser le seuil d'admission (85, `app/dsp/dreams.cpp`).
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
- [ ] MIDI / PERFORMANCE, K3 NOTES OUT = TRACKS : la MPC reçoit chaque piste sur son canal.
- [ ] GLO > MIDI CHANNELS, K4 ALL FROM = 3 : la MPC joue les pistes 1 à 6 sur les canaux 3 à 8, et les reçoit sur ces canaux ; rien n'arrive sur les canaux 1 et 2 (MS-20, 2600). Une piste mise sur OFF en sortie n'envoie plus rien. Les points IN et OUT de la page s'allument avec les notes.
- [ ] K3 = ZONES, avec les zones par défaut (C1–B2 canal 1, C3–B4 canal 2, C5–C7 canal 3) : chaque note arrive sur le canal de sa zone ; zones qui se chevauchent = note sur les deux canaux.
- [ ] K4 CLOCK OUT : la MPC suit le tempo, démarre et s'arrête avec PLAY.
- [ ] Gamme microtonale (Rast) en TRACKS : la MPC reçoit le pitch bend (régler sa plage sur ±2 demi-tons) ; en ZONES : notes arrondies, pas de bend.
- [ ] À l'arrêt et en PANIC : aucune note ne reste bloquée sur la MPC.
- [ ] Page MIDI : le satellite émet une onde à chaque message envoyé.

## 6. Audio par USB
- [ ] L'ordinateur voit le FM-1 comme entrée audio et l'enregistre (44,1 kHz stéréo), sans trou.
- [ ] Optionnel, plus tard : compiler avec `usb_audio_out = true` et vérifier que l'ordinateur joue à travers le FM-1, en surveillant la page SYSTEM.

## 7. Sauvegardes
- [ ] SAVE à l'arrêt : « MONDE + PREFS SAVED ». Éteindre, rallumer : le Monde et les préférences (MIDI out, canaux des pistes, zones, gamme CUSTOM) reviennent. Après l'installation par-dessus une version de Monades sans les canaux, les réglages MIDI et zones déjà sauvés restent, et les pistes sont sur les canaux 1 à 6.
- [ ] GLO > USER BANKS : sauver un Monde et un son, les recharger.
- [ ] Si une sauvegarde `.mddb` de Mutable Dreams existe : la restaurer (`python3 tools/dreams_backup.py restore … --port 'Monades FM-1'`, puis SEL sur le FM-1). Les Mondes de Mutable Dreams doivent se charger, toutes les nouveautés au neutre.

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
- [ ] Firmware d'origine : vérifier le SHA-256 du fichier V15 (`db1642b2b6fa5c2cccb11ffd13878068bb28601678…`, voir `docs/RELEASE-VALIDATION.md`), puis `python3 base/tools/fm1_install.py --yes FM-1-V15.fwsc`, d'abord sans `--force` : l'outil reconnaît le V15 exact à son empreinte. `--force` retire cette vérification. V15 efface les banques utilisateur : garder la sauvegarde `.mddb`.
- [ ] Réinstaller Monades (par exemple après le mode sans échec) : `python3 base/tools/fm1_install.py Monades-0.1.0.fwsc --yes`. Il n'y a pas encore de page d'installation web pour Monades : l'écran SAFE MODE parle de l'installeur web, mais c'est `fm1_install.py` qui sert.

## Ce qu'il faut me rapporter
Les chiffres des sections 1 et 3, les écrans peu lisibles, et tout ce qui coince (son, MIDI, sauvegardes), avec la page et les réglages.
