#!/usr/bin/env perl
# Brian Chrzanowski
# Mon Nov 18, 2019 01:28
#
# A test perl script to initialize the initial velocity

use warnings;
use strict;

# $|++;

my @dim;

for (my $lineno = 0; <STDIN>; $lineno++) {
	my $line = $_;
	chomp $line;

	my @arr = split(/\t/, $line);

	if ($lineno == 0) {
		@dim = map { "$_" + 0 } @arr;
	} else {
	}

	print "$lineno\n";
}

