#!/bin/bash
# creates a database of size 100 * MAX and runs ttdb
# writes the times to stdout
echo "# 1. files in database
# 2. wall time
# 3. CPU time
"

for i in `seq $1` ; do
    echo -n "$i "
    ./generate_testdb.pl test.db 100 100 $(( $i * 100))
    /usr/bin/time -f "%E %S" ./ttdb 2>&1
done
