#!/bin/bash

function cleanup_old_tests()
{
	rm -r step_*.tmp
}

TESTS="plugin_test core_test smoke_test functionality_test"

cleanup_old_tests

export LD_LIBRARY_PATH=bin/

for test in $TESTS; do
	bin/$TEST
done
