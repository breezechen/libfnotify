#!/bin/bash

# Die with some kind of message
function die()
{
	echo $@ >&2
	exit 1
}

# Remove old data that may interfere with running the tests
function cleanup_old_tests()
{
	# Cleanup for functionality_test
	rm -r step_* 2> /dev/null
}

if [[ -n $DEBUG ]]; then
	echo "Running tests through gdb"
	USE_GDB=gdb
else
	USE_GDB=
fi

TESTS="plugin_test core_test smoke_test functionality_test"
BIN_DIR=bin

case $(uname) in
	"Linux")
		NUM_CPUS=$(cat /proc/cpuinfo | grep '^processor	: [0-9][0-9]*$' | wc -l)
		;;
	*)
		NUM_CPUS=1
		;;
esac

NUM_THREADS=$((NUM_CPUS+1))

echo "Compiling with $NUM_THREADS threads"
make -j$NUM_THREADS >/dev/null 2>&1 || die "Failed to compile project"

cd "$BIN_DIR"

cleanup_old_tests

export LD_LIBRARY_PATH=.

FAILED=
SUCCEEDED=
NUM_FAILED=0
NUM_SUCCEEDED=0
NUM_TESTS=0

for test in $TESTS; do
	NUM_TESTS=$((NUM_TESTS+1))

	test_path=./$test
	echo "Running $test_path"
	$USE_GDB $test_path
	if [[ $? -ne 0 ]]; then
		FAILED="$FAILED $test"
		NUM_FAILED=$((NUM_FAILED+1))
	else
		SUCCEEDED="$SUCCEEDED $test"
		NUM_SUCCEEDED=$((NUM_SUCCEEDED+1))
	fi	
done

echo "Failed: $NUM_FAILED / $NUM_TESTS: $FAILED"
echo "OK    : $NUM_SUCCEEDED / $NUM_TESTS: $SUCCEEDED"
