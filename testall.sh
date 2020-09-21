#!/usr/bin/env bash
# Brian Chrzanowski
# 2020-09-21 16:55:06
#
# A Small Script to TEST ALL THE THINGS

TESTDIR="tests"
DEFAULTSZ=100
DEFAULTTM=10

# usage : print usage info
function usage
{
	echo "USAGE: $0"
	echo ""
	echo "  help                       this help message"
	echo ""
	echo "  clean                      cleans the test directory"
	echo ""
	echo "  listtests                  lists the available tests to run"
	echo ""
	echo "  add <mode> (x (y (z (t)))) adds a test of a dim^3 size to the test directory"
	echo "                             assumes '100' for spacial and '10' for temporal dimensions"
	echo ""
	echo "  run <testdir>              runs the tests without up to date data"
	echo "Ex."
	echo "  testall.sh add serial 100 50 40 10            # adds a cpu serial test"
	echo "  testall.sh add parallel-cpu 1000 1000 1000    # adds a cpu parallel test"
	echo "  testall.sh add parallel-gpu 899 988 123 5000  # adds a gpu parallel test"
	echo ""
	echo "  testall.sh run tests/                         # runs the tests in 'tests/'"
	echo ""
	exit 1
}

# add : makes a molt config file for the given size (L x W x H)
function add
{
	echo "in func $@"

	# check that we have length, width, height
	if [[ "$#" -lt 1 ]]; then
		echo "add needs 1 argument"
		exit 1
	fi

	if [ ! -f "./template.cfg" ]; then
		echo "Error: Couldn't read './template.cfg' to make a special config file"
		exit 1
	fi
}

# clean : cleans up the tests
function clean
{
	rm -rf $TESTDIR
}

# runall : run all of the tests
function runall
{
	if [[ "$#" -lt 1 ]]; then
		echo "run needs to be given the directory with your tests"
		exit 1
	fi
}

[ ! -d $TESTDIR ] && mkdir -pv $TESTDIR

# handle arguments
case "$1" in
	"clean")
		clean
	;;

	"add")
		shift
		add "$@"
	;;

	"run")
		runall
	;;

	"help")
		usage
	;;

	*)
		usage
	;;
esac

