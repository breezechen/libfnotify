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
	rm -r step_*.tmp 2> /dev/null
}

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

for test in $TESTS; do
	test_path=./$test
	echo "Running $test_path"
	$test_path
done
