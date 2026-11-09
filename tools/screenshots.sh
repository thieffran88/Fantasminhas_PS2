#!/bin/sh
# Gera capturas de tela do jogo (build de PC) em screenshots/*.png
set -e
cd "$(dirname "$0")/.."
B=./build/prefeitos_host
OUT=screenshots
mkdir -p "$OUT"
$B w40 @$OUT/01_titulo.ppm >/dev/null
$B D D D X w2 @$OUT/02_regras.ppm R R @$OUT/03_regras_prefeitos.ppm >/dev/null
# meio de partida com jogadas automáticas: procura uma semente que chegue
# a 13 cartas sem perder
for s in $(seq 1 300); do
  if $B --seed $s X w5 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 \
       | grep -q "tela=2 jogadas=13 .*resultado=0"; then SEED=$s; break; fi
done
MID="X w5 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20 A w20"
$B --seed $SEED $MID w30 R @$OUT/04_turista.ppm X w2 @$OUT/05_jogada.ppm >/dev/null
$B --seed $SEED $MID ST @$OUT/06_pausa.ppm >/dev/null
# vitória (procura uma semente em que a jogada automática vence)
for s in $(seq 1 400); do
  W="X w5"; for k in $(seq 1 24); do W="$W A w20"; done
  if $B --seed $s $W w60 | grep -q "tela=4 jogadas=24 .*resultado=1"; then
    $B --seed $s $W w60 @$OUT/08_vitoria.ppm >/dev/null; break; fi
done
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
