#!/bin/bash
# creates a database of size 100 * MAX and runs ttdb
# writes the times to stdout
echo "# 1. files in database
# 2. wall time
# 3. CPU time
"

for i in `seq $1` ; do
    echo -n "$i "
    # the average file is between 7 and 10 directories in
    # depth (based on my home directory on the high end
    # and the /usr directory on the lower end).
    # the number of tags would probably be a bit (but not
    # too much) higher since users don't have to choose
    # between like tags at the same level of specificity
    ./generate_testdb.pl test.db $((100000 * $i)) 124257 20
    /usr/bin/time -f "%E %S" ./ttdb 2>&1
done
