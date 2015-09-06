package Util;
# author: Mark Watts <mark.watts@utexas.edu>
# date: Wed Feb 11 23:33:30 CST 2015

use strict;
use warnings;
use Exporter;
our @ISA = qw/Exporter/;
our @EXPORT_OK = qw /natatime random_string/;
our @EXPORT = qw /natatime random_string/;

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

sub random_string
{
    my ($len, $chars) = @_;
    if (not defined $chars)
    {
        $chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    }
    my $res = "";
    my $charlen = length($chars);
    for (my $i = 0; $i < $len; $i++)
    {
        $res .= substr($chars, int(rand($charlen)), 1);
    }
    $res;
}

1;
