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
	echo "  clean                      removes generated config, log, and data files"
	echo ""
	echo "  squish                     squishes all of the logs for plotting time-series data"
	echo ""
	echo "  add <mode> (x (y (z (t)))) adds a test of a dim^3 size to the test directory"
	echo "                             assumes '100' for spacial and '10' for temporal dimensions"
	echo ""
	echo "  run <testdir>              runs the tests without up to date data"
	echo "Ex."
	echo "  testall.sh add 100 50 40 10            # adds a cpu serial test"
	echo "  testall.sh add ./moltthreaded.so 1000 1000 1000 10 # adds a cpu parallel test"
	echo "  testall.sh add ./molttcuda.so    1000 1000 1000 10 # adds a gpu parallel test"
	echo ""
	exit 1
}

# clean : removes config files, log files, and dat files (not the defaults)
function clean
{
	rm -f *_*.{cfg,log,dat}
}

# squish : squishes all of the files into one output stream (for what I care about)
function squish
{
	grep "" *.log | sed 's/:/\t/' | grep -E "_params|Step|_scale" | \
		grep "Elapsed Time" | awk '{print $1, $3, $7}' | \
		perl -nE 'BEGIN { say "Method,Volume,Timestep,Elapsed Time" } chomp; @a = split(/ /); $a[0] =~ /(.+)_(\w+)_/; $binary = $1; $vol = $2; say "$binary,$vol,$a[1],$a[2]";' | \
		(read -r ; printf "%s\n" "$REPLY"; sort -t , -k 1 -k 2n -k 3n)
}

# run : runs simulations for all of the config files present
function run
{
	for f in $(find . -type f -iname "*.cfg" ! -name "default.cfg" ! -name "template.cfg"); do
		DAT="${f%.cfg}.dat"
		LOG="${f%.cfg}.log"

		./molt --config "$f" -v "$DAT" > "$LOG"
		echo "$DAT"
	done
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

	# check to see if we have a library
	RE='^[0-9]+$'
	if [[ "$1" =~ $RE ]]; then
		THELIBRARY=""
	else
		THELIBRARY="$1"
		shift
	fi

	# get the volume
	VOL="$(($1 * $2 * $3))"
	[ -z "$THELIBRARY" ] && OUTFILE="default_${VOL}_${4}.cfg" || OUTFILE="${THELIBRARY}_${VOL}_${4}.cfg"

	# make the config file
	sed "s~VAR_X~$1~; s~VAR_Y~$2~; s~VAR_Z~$3~; s~VAR_T~$4~; s~VAR_LIBRARY~$THELIBRARY~;" < $TEMPLATE > $OUTFILE
}

# handle arguments
case "$1" in
	"clean")
		clean
	;;

	"add")
		shift
		# make sure we have a directory to add tests into
		[ ! -d "$(dirname $1)" ] && mkdir -pv "$(dirname $1)"
		add "$@"
	;;

	"run")
		run
	;;

	"squish")
		squish
	;;

	"help")
		usage
	;;

	*)
		usage
	;;
esac

