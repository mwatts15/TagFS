#!/usr/bin/perl
use File::Path qw(make_path);
use Cwd 'abs_path';

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
    @files = map { abs_path($_); } @files;
    for $file (@files)
    {
        my $dir = `dirname "$file"`;
        my $base = `basename "$file"`;
        chomp $dir;
        chomp $base;
        chdir $dir;
        if (&mkpth($tags))
        {
            rename($file, "$tags/$base");
        }
        else
        {
            die "Couldn't set up tags";
        }
    }
}
else
{
    print STDERR "A list of tags, separated by '/', and one or more file names must be provided";
    exit 1;
}
