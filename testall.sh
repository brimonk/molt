#!/usr/bin/env bash
# Brian Chrzanowski
# 2020-09-21 16:55:06
#
# A Small Script to TEST ALL THE THINGS

TEMPLATE="./template.cfg"

DEFAULTSZ=100
DEFAULTTM=10

MOLTEXE="./molt.exe"

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

function gettest
{
	[ -f "$1" ] && echo $1 || echo ""
}

# getfilets : returns the file's timestamp or -1 if none was found
function getfilets
{
	[ -f "$1" ] && stat -c "%Y" $1 || echo "-1"
}

# add : makes a molt config file for the given size (L x W x H)
function add
{
	# check that we have length, width, height
	if [[ "$#" -lt 1 ]]; then
		echo "add needs 1 argument"
		exit 1
	fi

	if [ ! -f $TEMPLATE ]; then
		echo "Error: Couldn't read '$TEMPLATE' to make a special config file"
		exit 1
	fi

	# file test x y z t
	realtest=$(gettest $2)

	# make the config file
	sed "s~VAR_T~$6~; s~VAR_X~$3~; s~VAR_Y~$4~; s~VAR_Z~$5~; s~VAR_LIBRARY~$realtest~;" \
		< $TEMPLATE > "$1.cfg"
}

# clean : cleans up the tests
function clean
{
	rm -rf $1
}

# runall : run all of the tests
function runall
{
	if [[ "$#" -lt 1 ]]; then
		echo "run needs to be given the directory with your tests"
		exit 1
	fi

	if [ ! -d "$1" ]; then
		echo "run needs to be given a directory"
		exit 1
	fi

	for i in $(find $1 -name "*.cfg");
	do
		odat="${i%.cfg}.dat"
		olog="${i%.cfg}.log"

		tsodat=$(getfilets $odat)
		tsolog=$(getfilets $olog)
		tsinpt=$(getfilets $i)

		if (( $tsinpt > $tsodat )); then
			echo $i
			$MOLTEXE --config $i -v $odat > $olog
			echo $odat
		fi
	done
}

# handle arguments
case "$1" in
	"clean")
		shift
		[ ! -d "$(dirname $1)" ] && mkdir -pv "$(dirname $1)"
		clean "$@"
	;;

	"add")
		shift
		# make sure we have a directory to add tests into
		[ ! -d "$(dirname $1)" ] && mkdir -pv "$(dirname $1)"
		add "$@"
	;;

	"run")
		shift
		# make sure we have a directory to add tests into
		[ ! -d "$(dirname $1)" ] && mkdir -pv "$(dirname $1)"
		runall "$@"
	;;

	"help")
		usage
	;;

	*)
		usage
	;;
esac

