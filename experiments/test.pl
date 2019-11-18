#!/usr/bin/env perl
# Brian Chrzanowski
# Mon Nov 18, 2019 01:28
#
# A test perl script to initialize the initial velocity
#
# This program operates in cooperation with the main 'molt' program.
# The main program, based on the configuration file, will fork() and create
# a child. The child will run your program! Assuming this "test.pl" program
# is noted in the 'initvel' or 'initamp' config lines, it will be executed.
#
# The simulator program expects these children programs to accept stdin, and
# write to stdout. What does it read and write from stdin and stdout? Floating
# point numbers of course! The main program will send, on the first line,
# the dimensionality of the simulation, tab delimited, in x, then y, then z.
# Following that, every line will be a grid point in the simulation that
# the main program expects will be replied to with another line, the value
# the initializer program wants to be in that location in the simulation.
#
# TODO Future Work (brian)
# 1. It could be really useful to have this program "know" more about the
#    main program. For instance, it might be useful to have this program ALSO
#    scan the config file for simulation parameters. Or, for the main program
#    to feed it more than the max dimensions of the program.
#
# 2. It could be nice to make the main program handle failures better.
#    Currently, I'm pretty sure that if this program ended early, the main
#    program would also crash. It isn't a problem if you just expect to follow
#    the format, but it might be nice for debugging crashes.


use warnings;
use strict;

my $C0 = 299792458;
my $EI = 4;
my $C = $C0 / sqrt($EI);

my @dim;

my ($mode) = @ARGV;

if (not defined $mode) {
	die "USAGE : $0 [--vel] [--amp]\n";
}

for (my $lineno = 0; <STDIN>; $lineno++) {
	my $line = $_;
	chomp $line;

	my $ans = 0;

	my @arr = split(/\t/, $line);
	@arr = map { "$_" + 0 } @arr;

	if ($lineno == 0) {
		@dim = @arr;
	} else {
		if ($mode eq "--vel") {
			$ans = funcg($arr[0], $arr[1], $arr[2]);
		} else {
			$ans = funcf($arr[0], $arr[1], $arr[2]);
		}
		# we only want to print when we get actual x,y,z points, not metadata
		print "$ans\n";
	}
}

# funcf : the f function from the initial matlab implementation
sub funcf
{
	my ($x, $y, $z) = (@_);

	my $xinit = (2 * $x / $dim[0] - 1) ** 2;
	my $yinit = (2 * $y / $dim[1] - 1) ** 2;
	my $zinit = (2 * $z / $dim[2] - 1) ** 2;

	my $interim = exp(-13.0 * ($xinit + $yinit + $zinit));

	return $interim;
}

# funcg : the g function from the initial matlab implementation
sub funcg
{
	my ($x, $y, $z) = (@_);

	my $interim = funcf($x, $y, $z);

	return $C * 2 * 13 * 2 / $dim[0] * (2 * $x / $dim[0] - 1) * $interim;
}

