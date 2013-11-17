valgrind --leak-check=full --show-reachable=yes -v ./run.sh > mem_check.out 2>&1
less mem_check.out
