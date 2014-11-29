#!/bin/sh

DIR=$(cd "$(dirname "$0")"; pwd)
./openkb \
 --rootdir "${DIR}/../Resources/data" \
 --datadir "${HOME}/Library/OpenKB" \
 --savedir "${HOME}/Library/OpenKB/saves" \
 --configdir "${HOME}/Library/OpenKB"

