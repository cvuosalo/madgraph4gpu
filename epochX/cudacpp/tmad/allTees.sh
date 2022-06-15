#!/bin/bash

short=0
makeclean=
keeprdat=-keeprdat

while [ "$1" != "" ]; do
  if [ "$1" == "-short" ]; then
    short=1
    shift
  elif [ "$1" == "-makeclean" ]; then
    makeclean=$1
    shift
  else
    echo "Usage: $0 [-short] [-makeclean]"
    exit 1
  fi
done

if [ "$short" == "1" ]; then
  ./tmad/teeMadX.sh -eemumu -ggtt -ggttg -ggttgg $makeclean $keeprdat
else
  ./tmad/teeMadX.sh -eemumu -ggtt -ggttg -ggttgg -ggttggg $makeclean $keeprdat
fi