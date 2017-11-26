#!/bin/sh -e

DIR="$(dirname "$(readlink -f "$0")")"

croak () {
    echo "$@" >&2
    exit 1
}

run_fuzzer () {
    if [ $# -lt 1 ]; then
        croak "Please, provide the input data TAR file. Given $# arguments: $@"
    fi
    DATA="$(readlink -f "$1")"
    RESD="${2:-fuzz_out}"
    SEED=${3:-1}
    ITER=${4:-1}
    NTHR=${5:-1}

    RESD="$(readlink -f "$RESD")"
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
                echo gdb src/fuzzer $TEMPD/core >> $FUZLOG
            fi
            echo ----- >> $FUZLOG
        fi
        cat $FUZLOG
    done
}

usage ()
{
    cat <<HERE
USAGE:
fuzzer run <data> [<result_dir>] [<num_seed>] [<number_of_op_rounds>] [<number_of_threads>]
    run the fuzzer with the given input <data>, writing results to
    <result_dir>, with random number generator seeds from 1 to <num_seed>,
    inclusive. <number_of_op_rounds> and <number_of_threads> are passed to \`fuzzer\`
HERE

}

case "$1" in 
    run)
        shift
        echo 'Running fuzzer...'
        run_fuzzer "$@"
        echo 'Done.'
        ;;
    *)
        usage
        ;;
esac
