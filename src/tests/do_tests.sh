#!/bin/sh

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

all_tests=test_*
tests=${TESTS:-$all_tests}
test_results_dir="${TEST_RESULTS_DIR:-unit-test-results}"
mkdir -p "$test_results_dir"
for x in $tests ; do 
    if [ -x $x ] ; then
        echo "$x"
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
            EXIT_STATUS=1
            cat $test_out
        fi

        if [ ! $NO_VALGRIND ] ; then
            grep --silent -e "ERROR SUMMARY: 0 errors" $valgrind_out; last_status=$?
            if [ $last_status -ne 0 ] ; then
                EXIT_STATUS=1
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
        echo
    fi
done

exit $EXIT_STATUS
