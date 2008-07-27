#!/bin/bash

function cleanup_old_tests()
{
	rm -r step_*.tmp
}

TESTS="plugin_test core_test smoke_test functionality_test"
BIN_DIR=bin

cleanup_old_tests

cd "$BIN_DIR"

export LD_LIBRARY_PATH=.

for test in $TESTS; do
	test_path=./$test
	echo "Running $test_path"
	$test_path
done
