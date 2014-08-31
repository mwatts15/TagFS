#!/bin/bash

#TESTS=( t_code_table t_hash_sets t_query t_result_queue t_tagdb t_tokenizer )
#n=${#TESTS[@]}
#for ((i=0;i<$n;i++)) ; do
    #testname=${TESTS[${i}]}
    #echo `echo ${testname/t_/Testing }| tr '_' ' '`
    #./$testname | egrep -i "PASS|FAIL" | sed -E "s/^/    /"
    #echo
#done
if [ $NO_VALGRIND ] ; then
    VALGRIND_COMMAND=""
else
    VALGRIND_COMMAND="valgrind --suppressions=valgrind-suppressions --leak-check=full "
fi
all_tests=test_*
tests=${TESTS:-$all_tests}
for x in $tests ; do 
    if [ -x $x ] ; then
        echo "$x"
        export G_DEBUG=gc-friendly 
        export G_SLICE=always-malloc
        valgrind_out=`mktemp valgrind-outXXX`
        $VALGRIND_COMMAND ./$x 2>$valgrind_out | egrep "Test:.*FAIL|[Ss]egmentation +[fF]ault" | sed -E "s/^/    /"
        grep --silent -e "ERROR SUMMARY: 0 errors" $valgrind_out
        if [ $? -ne 0 ]; then
            cat $valgrind_out
        fi
        rm $valgrind_out
        echo
    fi
done
