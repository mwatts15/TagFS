#!/bin/sh

EXIT_STATUS=0
if [ $NO_VALGRIND ] ; then
    VALGRIND_COMMAND=""
else
    VALGRIND_COMMAND="valgrind --suppressions=valgrind-suppressions --leak-check=full "
fi

if [ $NO_FILTER ] ; then
    filter_cmd ()
    {
        cat
    }
else
    filter_cmd ()
    {
        egrep -e 'Test:.*FAIL|[Ss]egmentation +[fF]ault'
    }
fi


all_tests=test_*
tests=${TESTS:-$all_tests}

for x in $tests ; do 
    if [ -x $x ] ; then
        echo "$x"
        export G_DEBUG=gc-friendly 
        export G_SLICE=always-malloc
        valgrind_out=`mktemp valgrind-outXXX`
        $VALGRIND_COMMAND ./$x 2>$valgrind_out | filter_cmd ; filter_status=$?
        if [ $filter_status -eq 0 ] ; then
            EXIT_STATUS=1
        fi

        grep --silent -e "ERROR SUMMARY: 0 errors" $valgrind_out; last_status=$?

        if [ $last_status -ne 0 ]; then
            EXIT_STATUS=1
            cat $valgrind_out
        fi
        rm $valgrind_out
        echo
    fi
done

exit $EXIT_STATUS
