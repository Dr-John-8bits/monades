# Fichiers Scala

Les fichiers `.scl` (format [Scala](https://www.huygens-fokker.org/scala/scl_format.html)) déposés ici sont ajoutés à la famille **SCALA** du sélecteur de gammes.

```sh
python3 tools/generate_scales.py app/dsp/scale_table.inc
```

Ensuite, recompiler le firmware. Le nom affiché vient du nom du fichier (13 caractères au plus).

- Hauteurs en ratios (`3/2`) ou en cents (`701.955`), la dernière ligne est la période (souvent `2/1`, mais `3/1` marche aussi, comme pour Bohlen-Pierce).
- 31 degrés au plus par période.
- En sortie MIDI, une note microtonale part avec un pitch bend (plage supposée de ±2 demi-tons) en mode TRACKS ; en mode ZONES, elle est arrondie au demi-ton le plus proche.

Exemples fournis : `shruti22.scl` (22 shrutis indiens, une lecture courante en intonation juste) et `bohlen_pierce.scl`.
