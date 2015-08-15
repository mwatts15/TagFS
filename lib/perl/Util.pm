package Util;
# author: Mark Watts <mark.watts@utexas.edu>
# date: Wed Feb 11 23:33:30 CST 2015

use strict;
use warnings;
use Exporter;
our @ISA = qw/Exporter/;
our @EXPORT_OK = qw /natatime/;
our @EXPORT = qw /natatime/;

sub natatime ($@)
{
    # Take an array and return an iterator that returns, at most, n
    # objects at a time

    my $n = shift;
    my @list = @_;

    return sub
    {
        return splice @list, 0, $n;
    }
}
1;
