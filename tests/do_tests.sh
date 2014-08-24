#!/bin/bash

#TESTS=( t_code_table t_hash_sets t_query t_result_queue t_tagdb t_tokenizer )
#n=${#TESTS[@]}
#for ((i=0;i<$n;i++)) ; do
    #testname=${TESTS[${i}]}
    #echo `echo ${testname/t_/Testing }| tr '_' ' '`
    #./$testname | egrep -i "PASS|FAIL" | sed -E "s/^/    /"
    #echo
#done

for x in test_* ; do 
    if [ -x $x ] ; then
        #testname=${TESTS[${i}]}
        #echo `echo ${testname/t_/Testing }| tr '_' ' '`
        #./$x
        ./$x | egrep "Test:.*FAIL|Segmentation" | sed -E "s/^/    /"
    fi
done
