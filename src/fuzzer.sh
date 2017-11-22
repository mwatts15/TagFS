#!/bin/sh -e

DIR="$(dirname "$(readlink -f "$0")")"
DATA="$(readlink -f "$1")"
if [ $# -ne 5 ]; then
    exit 1
fi
RESD="$2"
SEED=$3
ITER=$4
NTHR=$5

mkdir -p "$RESD"

ulimit -c unlimited
for x in $(seq $SEED) ; do
    FAIL=0
    TEMPD=$(mktemp -p "$RESD" -d datadir-$x-XXXXXXXXX)
    FUZLOG="$TEMPD/fuzzer.log"
    tar xf "$DATA" -C $TEMPD/ 
    $DIR/fuzzer --data-dir="$TEMPD" $x $ITER $NTHR 2>>$FUZLOG || FAIL=1
    if [ $FAIL -eq 1 ] ; then
        echo FAIL- | tee >> $FUZLOG
        echo $TEMPD >> $FUZLOG
        if [ -f core ] ; then
            mv core $TEMPD/core
        fi
        echo ----- >> $FUZLOG
    fi
done
