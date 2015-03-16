#!/usr/bin/perl
use File::Path qw(make_path);

sub mkpth
{
    # XXX: Augments make_path to retry
    my $arg = shift;
    my $err;
    make_path($arg, {error => \$err});

    if (@$err) {
        for my $diag (@$err) {
            my ($file, $message) = %$diag;
            print "error creating tags ($file): $message\n";
        }
        return 0;
    }
    return 1;
}

if (scalar(@ARGV) > 1)
{
    my $tags = pop @ARGV;
    my @files = @ARGV;
    if (&mkpth($tags))
    {
        for $file (@files)
        {
            my $base = `basename "$file"`;
            chomp $base;
            rename($file, "$tags/$base");
        }
    }
}
else
{
    print STDERR "A list of tags, separated by '/', and one or more file names must be provided";
    exit 1;
}
