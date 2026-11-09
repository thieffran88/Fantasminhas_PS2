#!/bin/sh
# Gera capturas de tela do jogo (build de PC) em screenshots/*.png
set -e
cd "$(dirname "$0")/.."
B=./build/prefeitos_host
OUT=screenshots
mkdir -p "$OUT"
rm -f "$OUT"/*.png

# abertura (capa) e menu; X na abertura abre o menu, X no menu começa a partida
$B w10 @$OUT/01_abertura.ppm >/dev/null
$B X w2 @$OUT/02_menu.ppm >/dev/null
$B X D D D D X w2 R R @$OUT/03_regras.ppm >/dev/null

# meio de partida com jogadas automáticas: procura uma semente que chegue
# a 13 cartas sem perder
MID="X X w5"
for k in $(seq 1 13); do MID="$MID A w20"; done
SEED=1
for s in $(seq 1 300); do
  if $B --seed $s $MID | grep -q "tela=2 jogadas=13 .*resultado=0"; then SEED=$s; break; fi
done
$B --seed $SEED $MID w30 R @$OUT/04_turista.ppm X w2 @$OUT/05_jogada.ppm >/dev/null
$B --seed $SEED $MID ST @$OUT/06_pausa.ppm >/dev/null

# derrota: joga sempre na primeira cidade livre até perder
END="X X w5"
for k in $(seq 1 24); do
  END="$END X X w20"
  if $B --seed 3 $END w60 | grep -q "tela=4"; then break; fi
done
$B --seed 3 $END w60 @$OUT/07_derrota.ppm >/dev/null

# vitória: procura uma semente em que a jogada automática vence
W="X X w5"
for k in $(seq 1 24); do W="$W A w20"; done
for s in $(seq 1 400); do
  if $B --seed $s $W w60 | grep -q "tela=4 jogadas=24 .*resultado=1"; then
    $B --seed $s $W w60 @$OUT/08_vitoria.ppm >/dev/null; break; fi
done

# outros idiomas: R na abertura troca o idioma (inglês, depois espanhol)
$B R w8 @$OUT/09_abertura_en.ppm X w2 @$OUT/10_menu_en.ppm >/dev/null
MID_ES="R R X X w5"
for k in $(seq 1 13); do MID_ES="$MID_ES A w20"; done
$B --seed $SEED $MID_ES w30 X w2 @$OUT/11_jogada_es.ppm >/dev/null
MID_EN="R X X w5"
for k in $(seq 1 13); do MID_EN="$MID_EN A w20"; done
$B --seed $SEED $MID_EN w30 X w2 @$OUT/12_jogada_en.ppm >/dev/null

for f in $OUT/*.ppm; do
  python3 -c "import sys;from PIL import Image;Image.open(sys.argv[1]).save(sys.argv[1][:-4]+'.png')" "$f" && rm "$f"
done
ls $OUT
