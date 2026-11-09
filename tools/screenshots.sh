#!/bin/sh
# Gera capturas de tela do jogo (build de PC) em screenshots/*.png
set -e
cd "$(dirname "$0")/.."
B=./build/prefeitos_host
OUT=screenshots
mkdir -p "$OUT"
$B w40 @$OUT/01_titulo.ppm >/dev/null
$B D D D X w2 @$OUT/02_regras.ppm R R @$OUT/03_regras_prefeitos.ppm >/dev/null
# meio de partida: alterna fundar cidades e completar as existentes
MID="X w5"
for k in 1 2 3 4 5 6 7 8 9; do
  case $k in 1|3|5|8) MID="$MID X X w20" ;; 2|6) MID="$MID R X X w20" ;; *) MID="$MID R X D X w20" ;; esac
done
$B --seed 11 $MID R X @$OUT/04_turista.ppm L X w2 R @$OUT/05_jogada.ppm >/dev/null
$B --seed 11 $MID ST @$OUT/06_pausa.ppm >/dev/null
# joga sempre na primeira cidade livre até a partida acabar (tela final)
END="X w5"
for k in $(seq 1 24); do
  END="$END X X w20"
  if $B --seed 3 $END w60 | grep -q "tela=4"; then break; fi
done
$B --seed 3 $END w60 @$OUT/07_fim.ppm >/dev/null
for f in $OUT/*.ppm; do
  python3 -c "import sys;from PIL import Image;Image.open(sys.argv[1]).save(sys.argv[1][:-4]+'.png')" "$f" && rm "$f"
done
ls $OUT
