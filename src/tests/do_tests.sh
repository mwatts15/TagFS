#!/bin/bash

EXIT_STATUS=0
if [ $NO_VALGRIND ] ; then
    VALGRIND_COMMAND=""
else
    VALGRIND_COMMAND="valgrind --suppressions=valgrind-suppressions --leak-check=full "
fi
if [ $TESTS_MACHINE_OUTPUT ] ; then
    RUNNER_TYPE=xml
else
    RUNNER_TYPE=basic
fi

DO_FORK=1

if [ $COVERAGE ] ; then
    DO_FORK=
fi

do_test (){
    local x=$1
    local test_results_dir=$2
    local exist_status=0
    if [ -x $x ] ; then
        echo "--START $x--"
        this_test_result_dir="$test_results_dir/$x"
        mkdir -p $this_test_result_dir
        export G_DEBUG=gc-friendly 
        export G_SLICE=always-malloc
        test_out=`mktemp /tmp/tagfs_test-out.XXX`

        if [ ! $NO_VALGRIND ] ; then
            valgrind_out=`mktemp /tmp/valgrind-out.XXX`
            VALGRIND_COMMAND="$VALGRIND_COMMAND --log-file=$valgrind_out "
        fi
        $VALGRIND_COMMAND ./$x $RUNNER_TYPE > $test_out; last_status=$?
        # -- Calculate failures and show failure output
        if [ $last_status -ne 0 ] ; then
            exit_status=1
            cat $test_out
        fi

        if [ ! $NO_VALGRIND ] ; then
            grep --silent -e "ERROR SUMMARY: 0 errors" $valgrind_out; last_status=$?
            if [ $last_status -ne 0 ] ; then
                exit_status=1
                cat $valgrind_out
            fi
        fi

        # -- Move outputs to results directory
        if [ $TESTS_MACHINE_OUTPUT ] ; then
            mv $x-Listing.xml "$this_test_result_dir/"
            mv $x-Results.xml "$this_test_result_dir/"
        fi
        if [ ! $NO_VALGRIND ] ; then
            mv $valgrind_out "$this_test_result_dir/"
        fi
        mv $test_out "$this_test_result_dir/"
        echo "---STOP $x--"
    fi
    return $exit_status
}

all_tests=test_*
tests=${TESTS:-$all_tests}
test_results_dir="${TEST_RESULTS_DIR:-unit-test-results}"
mkdir -p "$test_results_dir"
EXIT_STATUS=0

child_count=0
for x in $tests ; do 
    if [ $DO_FORK ] ; then
        do_test "$x" "$test_results_dir" &
        child_count=$((child_count + 1))
    else
        do_test "$x" "$test_results_dir"
    fi 
done

while [ $child_count -gt 0 ] ; do
    wait -n
    if [ $? -ne 0 ] ; then
        EXIT_STATUS=1
    fi
    child_count=$((child_count - 1))
done

exit $EXIT_STATUS
