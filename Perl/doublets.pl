#!/usr/bin/perl
# Chris Walker
# Assignment 7 Doublets
# doublets.pl

# benchmarks on my machine:
#
# Start -> End		Execution time
# -----	   ---		--------------
# chaos -> order	24 seconds 
# charge -> comedo	52 seconds
# plant -> hovel	23 seconds
# plant -> cheat	12 seconds
# cattle -/> bbbbbb 52 seconds

use strict;
use Class::Struct;

# data structure that helps backtrace my way
# from the discovered end word
# back to the starting word
struct WordCombo => {
	new_word => '$',
	prev_word => '$',
};

my $first = $ARGV[0];
my $last = $ARGV[1];

# input validation
if (!$first || !$last || $ARGV[2]) {
	print("You must provide exactly two arguments!\n");
	exit 1;
}

if (length($first) != length($last)) {
	print("$first and $last must be the same length!\n");
	exit 1;
}

# open and read file
my $filename = 'wordlist.txt';
open(my $fh, '<:encoding(UTF-8)', $filename)
  or die "Could not open file '$filename' $!";

my @words;
my $noNeedToAddLastToWords = 0;

while (my $row = <$fh>) {	
	chomp($row);
	if (length($row) == length($first)) {
		push(@words, $row);
	}

	if ($row eq $last) {
		$noNeedToAddLastToWords = 1;
	}
}

# just in case user inputs a word that's not found in wordlist.txt;
# program crashes if $last is not in @words
# so we push $last onto @words if $last wasn't found in wordlist.txt
if (!$noNeedToAddLastToWords) {
	push(@words, $last); 
}
close($fh);

my @chain;
my @neighbors; 
my %alreadyChecked;

# returns true iff x is 1 letter apart from z
# O(N) where N is number of characters of $_[1]
sub one_letter_diff {
	my ($x, $z) = @_;

	my @xToArr = split('', "$x");
	my @zToArr = split('', "$z");
	my $countOfDiff = 0;

	foreach my $i (0 .. $#xToArr) {
		$countOfDiff++ if $xToArr[$i] ne $zToArr[$i];

		# return 0 (false) if more than 1 letter separates $x and $z
		return 0 if $countOfDiff gt 1; 
	}

	# returns 1 (true) if 1 letter separates $x and $z
	# returns 0 (false) if $x eq $z
	return $countOfDiff;
}

# main recursive function
# base case is O(N) where N == number of links in chain
# recursive case is 0(N*M) where N == Number of words in @words, and M == length of word
sub create_chain {
	my ($a) = @_;

	# base case, success
	if ($a->new_word eq $last) {

		# back trace to start word, creating the chain
		# in the process
		# Once chain is completed, print it then exit
		while ($a->prev_word ne $first) {
			unshift(@chain, $a->prev_word);
			$a->new_word($a->prev_word);
			$a->prev_word($alreadyChecked{$a->new_word});
		}
		unshift(@chain, $first);
		push(@chain, $last);
		print(join(' --> ', @chain));
		print("\n");
		exit 0;
	}
	else { 
		$alreadyChecked{$a->new_word} = $a->prev_word;

		# loop through words
		# get all words that are 1 letter away from $a
		# and haven't been checked
		# store in @neighbors
		# recursively call create_chain with an element from @neighbors, 
		# breadth-first search implementation.
		foreach my $word (@words) {
			if (!exists($alreadyChecked{$word}) && one_letter_diff($word, $a->new_word)) {
				my $newObj = WordCombo->new(new_word => $word, prev_word => $a->new_word);  
				push(@neighbors, $newObj);
				$alreadyChecked{$word} = $a;
			}	
		}

		# if @neighbors is empty and base case was not reached, 
		# that means the path can't exist. Print message and exit.
		if (!@neighbors) {
			print "There was no path found between $first and $last.\n";
			exit 0;
		}
		
		create_chain(shift(@neighbors));
	}
}

my $wc = WordCombo->new(new_word => $first, prev_word => '',);
create_chain($wc);
