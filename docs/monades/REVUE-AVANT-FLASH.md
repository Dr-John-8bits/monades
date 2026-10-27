# Revue avant le premier flash — consignes pour Claude

Tu fais une revue indépendante du firmware **Monades** avant qu'il soit installé pour la première fois sur un M-VAVE FM-1. Ton rôle : trouver ce qui pourrait empêcher le FM-1 de démarrer, le rendre inutilisable (« brick »), le faire planter, ou abîmer les données de l'utilisateur. Le reste (confort, style) passe après.

Écris ton rapport en français, simplement : la personne qui le lira n'est pas développeuse.

## Règles

- **Lecture seule.** Ne modifie aucun fichier du projet, ne fais ni commit ni push. Tu peux compiler et lancer les tests (ils écrivent seulement dans `app/build/`).
- **Ne flashe rien, ne branche rien.** N'exécute pas `base/tools/fm1_install.py` (ni aucun outil qui parle au FM-1 en MIDI ou en USB) autrement qu'en lisant son code.
- **N'installe pas la chaîne JieLi ni le SDK AC79** sans qu'on te le demande. La compilation pour l'appareil n'a jamais été faite : c'est une des choses à évaluer, pas à faire.
- **Ne lis pas `docs/monades/LORE.md`** s'il est présent : c'est un texte privé, sans rapport avec le code.
- Le site web (`site/`) est hors sujet, sauf `site/simulateur/` si tu en as besoin pour comprendre.
- Écris ton rapport dans un nouveau fichier `REVUE-RESULTATS.md` à la racine du dépôt (c'est le seul fichier que tu crées).

## Le projet en bref

- **Monades** est un fork de **Mutable Dreams 1.1** (Kid Bunka, GPL-3.0, `https://github.com/kidbee85-beep/mutable-dreams`, remote git `upstream`). Mutable Dreams, lui, a été installé et validé sur le FM-1 (voir `docs/RELEASE-VALIDATION.md`). **Aucune version de Monades n'a encore tourné sur l'appareil.**
- Matériel : M-VAVE FM-1, puce JieLi **AC791N** (cœur pi32v2), écran 240×240, 14 boutons, encodeurs, clavier. Contraintes connues : le moteur de Mutable Dreams prenait déjà environ 85 % du temps de calcul d'un cœur ; mémoire : **RAM .data+.bss 98 304 octets** (Mutable Dreams : 71 768) et **pool 344 064** (Mutable Dreams : 334 800).
- `base/` : la plateforme fm1-base (pilotes, installateur, simulateur de bureau `base/host/`, version web). `app/` : l'application Monades.
  - `app/dsp/dreams.cpp` et ses modules `app/dsp/*.inc` (une seule unité de compilation) : le moteur audio, qui tourne dans le fil audio.
  - `app/app.c` : l'écran, les boutons, les pages (fil de l'interface). Il inclut `banks.c`, `transfer.c`, `web.c`.
  - `app/fm1.toml` : la configuration de compilation. `tools/build_device.sh` : la compilation pour le FM-1.
  - `app/dsp/preset_bank.inc` est généré par `tools/generate_presets.py` (+ `tools/fdds_mondes.py`).
- Ce que Monades ajoute à Mutable Dreams : 91 gammes dont microtonales (`app/dsp/scale_table.inc`), sortie MIDI, LFO, générateurs, forme, gamakas, chorus, écho synchronisé, DICE, Mondes FDDS, morphing (`app/dsp/morph.inc`), dérive (`app/dsp/drift.inc`), nouvelles pages. Le format des patchs est passé de 1 292 à 1 680 octets (version 10) avec une chaîne de migration dans `md_decode`.
- L'historique est dans git (`git log`), la feuille de route dans `docs/monades/FORK.md`, la liste des tests à faire sur l'appareil dans `docs/monades/TESTS-FM1.md`, le mode d'emploi dans `site/manuel/index.html`. Pour voir tout ce que Monades change : `git diff upstream/main...main -- app tools` (si le remote `upstream` manque : `git remote add upstream https://github.com/kidbee85-beep/mutable-dreams.git && git fetch upstream`).

## Mise en route (macOS, Apple Silicon)

```bash
xcode-select --install          # si les outils de compilation manquent
python3 -m venv .venv && .venv/bin/pip install pillow
export SDKROOT=$(xcrun --show-sdk-path)
```

Compiler et lancer les tests (avec les sanitizers d'adresse et de comportement indéfini) :

```bash
P=.venv/bin/python
for t in host next dsp quality; do $P tools/build_local.py $t --sanitize; done
app/build/san/next-tests | tail -1
app/build/san/dsp-tests | tail -1
app/build/san/audio-quality-tests | tail -1
for s in ui next_ui mellow_ui context_help all_context_help help_library morph_ui derive_ui lights_ui; do
  app/build/san/mutable-dreams-host tests/$s.txt "$(mktemp -d)" >/dev/null 2>&1 && echo "OK $s" || echo "FAIL $s"
done
$P tools/test_transfer.py; $P tests/storage_banks.py
```

Le simulateur de bureau lit des scripts (`tap`, `press`, `turn`, `spin`, `wait`, `shot`, `expect`…) : voir l'en-tête de `base/host/plat_host.c` et les fichiers `tests/*.txt`. Tous ces tests passent sur le Mac de l'auteur ; dis-le si ce n'est pas le cas chez toi.

## Ce qu'il faut vérifier, par ordre d'importance

### 1. Le risque de brick (le plus important)
- Comment le firmware est empaqueté et installé : `tools/build_device.sh`, `base/tools/fm1pkg_make.py`, `base/tools/fm1_install.py`, `tools/package_*.py`, `app/fm1.toml` (nom, identité, version `0.1.0`, `FM-1_714000100`). Est-ce que l'installation touche au chargeur de démarrage, ou seulement à l'application ? Comment revient-on au firmware d'origine V15 (`docs/Recovery-Guide.txt`) ? Y a-t-il un cas où l'appareil ne pourrait plus recevoir de mise à jour ?
- Ce qui se passe au démarrage : lecture des préférences et des patchs sauvegardés (`plat_store_load`, `banks.c`, `transfer.c`, `md_decode`, `md_validate`). Un FM-1 qui a déjà des données de Mutable Dreams (ou du firmware d'origine) doit démarrer sans planter. Vérifie qu'une donnée invalide ou d'une autre taille est refusée proprement, jamais lue de travers.
- Tout ce qui écrit en mémoire flash (SAVE, USER BANKS, sauvegarde USB) : peut-on écrire hors zone, ou laisser une donnée à moitié écrite qui bloque le démarrage suivant ?

### 2. La mémoire et la pile
- **RAM et pool** : estime ce que Monades ajoute à Mutable Dreams (variables statiques, tables). L'auteur estime environ 88 000 octets de RAM et 339 000 de pool (mesure avec le compilateur du Mac, `size -m` sur `dsp_dreams_cpp.o`). Qu'est-ce qui va dans `.pool` (`POOL` dans `dreams.cpp`) et qu'est-ce qui va en RAM ? Les tables `const` vont-elles bien en flash et pas en RAM sur cette cible ?
- **La pile du fil audio** : cherche les grosses variables locales dans le code appelé depuis `md_render` / `apply_command` / `tick` (le fil audio). Les copies de `md_patch` (1 680 octets) qu'y faisaient le chargement d'un Monde, le choix du Monde B et la dérive ont été retirées (branche `monades/pile`) : mesuré avec `clang++ -O2 -fstack-usage` sur `app/dsp/dreams.cpp`, `tick` est passé de 5 248 à 336 octets, et le chemin le plus profond est `md_render` (160) → `quantum` (2 640) → rendu d'une voix (jusqu'à 896), contre `md_render` (2 544) pour Mutable Dreams 1.1. Vérifie qu'il ne reste rien de ce genre, et quelle taille de pile le fil audio a dans `base/` et le SDK : un dépassement ferait planter l'appareil.

### 3. Le temps de calcul
- Ce qui s'ajoute dans le fil audio par rapport à Mutable Dreams : chorus, diffusion, passe-haut des envois, LFO, gamakas, morphing (`morph_apply` parcourt les 420 mots du patch à chaque pas pendant un morphing), changements de moteur (`reset_voice`) pendant un morphing. Le garde-fou ECO / FULL (`patch.quality`, poids par moteur dans `dreams.cpp`) suffit-il ?

### 4. La concurrence entre le fil audio et l'interface
- L'interface lit l'état du moteur (`md_state()`, `md_status()`, `md_morph_view`, `md_drift_view`) pendant que le fil audio l'écrit, et envoie des commandes par une file (`md_command`, 64 places). Repère ce qui pourrait être lu à moitié écrit et avoir une conséquence autre qu'un affichage faux d'une image (indice hors tableau, pointeur, boucle infinie).

### 5. La justesse
- Les nouvelles commandes (`apply_command` dans `dreams.cpp`, de `MD_LFO_RATE` à `MD_DRIFT_HOME`) : bornes, indices de piste et de LFO, valeurs négatives.
- La migration des anciens patchs (`md_decode`) : chaque taille connue (652, 1 292, 1 512, 1 620, 1 676, 1 680 octets).
- Le morphing et la dérive : un patch produit doit toujours passer `md_validate` (longueur, coups, rotation…).
- Le MIDI : sortie par pistes et par zones, horloge, pitch bend microtonal (`midi_out.inc`), aucune note bloquée à l'arrêt ou au changement d'horloge.
- Tout comportement indéfini en C/C++ (débordements signés, décalages, lectures non initialisées), surtout ce qui pourrait se comporter différemment sur le compilateur JieLi que sur clang.

### 6. Le reste
Ce que tu remarques d'autre et qui mérite d'être corrigé avant de flasher.

## Le rapport attendu (`REVUE-RESULTATS.md`)

1. **Verdict** en une ou deux phrases : peut-on compiler et flasher en l'état, et le risque de brick est-il faible ?
2. **Bloquant avant de flasher** : ce qui peut planter l'appareil, l'empêcher de démarrer, abîmer les données ou rendre la mise à jour impossible.
3. **À corriger bientôt** : défauts réels mais sans danger pour l'appareil.
4. **Mineur** : le reste.
5. **Ce que tu n'as pas pu vérifier** (par exemple sans la chaîne JieLi), et comment le vérifier.

Pour chaque point : le fichier et la ligne (`app/dsp/dreams.cpp:123`), ce qui se passe concrètement, dans quelles conditions, et la correction que tu proposes. Ne signale que ce que tu as vérifié dans le code ; dis clairement quand c'est une supposition.
