#!/usr/bin/env perl

# This script generates a (presumably) correctly 
# structured test database by using an (allegedly) 
# correct tagfs. 
#
# It should only be used once all of the other tests
# which don't use the test database pass.

my $mountDirName;
my $dataDirName;
my $keepDataDir = 0;
my $DATABASE = "test.db";
my $MAX_FILES = 10; # The actual number of files that will be created
my $MAX_TAGS = 50; # The actual number of tags that will be created
my $MAX_TAGS_PER_FILE = 16; # Upper limit of tags per file. Distribiution is near-uniform.
my $MAX_SUPER_TAGS_PER_TAG = 4; # Upper limit of super tags for a tag.
my $SEED = 1234567;
my $TPS = "::"; # tag path separator. This must match the TAG_PATH_SEPARATOR in ../tag.h

($MAX_TAGS_PER_FILE < $MAX_TAGS) || die "You can't have more tags per file than you have tags.";
($MAX_SUPER_TAGS_PER_TAG < $MAX_TAGS) || die "You can't have more tags per file than you have tags.";

sub make_tempdir
{
    my $tail = shift;
    my $s = `mktemp -d /tmp/tagfs-testdb-${tail}-XXXXXXXXXX`;
    chomp $s;
    if (not (-d $s))
    {
        print "Couldn't create test directory\n";
        exit(1);
    }
    $s
}

sub setup
{
    srand $SEED;
    $mountDirName = make_tempdir("mount");

    if (not defined $dataDirName)
    {
        $dataDirName = make_tempdir("data");
    }
    my $cmd = "../tagfs --drop-db -g 0 --log-file generate.log --data-dir=$dataDirName -b $DATABASE $mountDirName";
    print("$cmd\n");
    (system($cmd) == 0) or die "Couldn't exec tagfs: $!\n";
}

sub new_file
{
    my $file = shift;
    if ( open F, ">", $file )
    {
        close F;
        return 1;
    }
    else
    {
        return 0;
    }
}

sub teardown
{
    while (`fusermount -u $mountDirName 2>&1` =~ /[Bb]usy/)
    {
        print "waiting to perform clean unmount...\n";
        sleep 1;
    }
    `rm -rf $mountDirName`;

    if (not $keepDataDir)
    {
        `rm -rf $dataDirName`;
    }
}

sub rand_lim
{
    my $limit = shift;
    return int(rand($limit));
}

sub generate
{
    make_files();
    make_tags();
    add_tags_to_files();
}

sub make_files
{
    for (my $file_idx = 0; $file_idx < $MAX_FILES; $file_idx++)
    {
        my $file_name = $file_idx + 1;
        my $file = "$mountDirName/$file_name";
        if (not new_file($file))
        {
            print STDERR "Couldn't create $file. Exiting.";
            die;
        }
    }
}

sub make_tags
{
    # Parent tag names are drawn from within $MAX_TAGS.
    # The actual subtag names are irrelevant though.
    for (my $tag_idx = 0; $tag_idx < $MAX_TAGS; $tag_idx++)
    {
        my $base_name = "d${tag_idx}";

        my %parents = ();
        my $num_tags = rand_lim($MAX_SUPER_TAGS_PER_TAG);
        while (scalar(keys(%parents)) < $num_tags)
        {
            my $tag = rand_lim($MAX_TAGS);
            if ($tag != $tag_idx)
            {
                $parents{$tag} = 1;
            }
        }
        my @parent_tags = keys(%parents);
        @parent_tags = map { "d$_" } @parent_tags;
        push(@parent_tags, $base_name);
        $base_name = join($TPS, @parent_tags);
        my $dir = "$mountDirName/$base_name";
        print "Dir name = $dir\n";
        if ((not (mkdir($dir))) and (not $!{EEXIST}))
        {
            print STDERR "Couldn't create $dir: $!";
            die;
        }
    }
}

sub add_tags_to_files
{
    for (my $file_idx = 0; $file_idx < $MAX_FILES; $file_idx++)
    {
        my $num_tags = rand_lim($MAX_TAGS_PER_FILE);
        my %tags = ();
        while (scalar(keys(%tags)) < $num_tags)
        {
            my $tag = rand_lim($MAX_TAGS);
            $tags{$tag} = 1;
        }

        foreach $tag_to_add (keys(%tags))
        {
            # XXX: This assumes that the name of the file is the same as its file_id
            # because of the way we created the files. This isn't a correct assumption
            # generally.
            rename "$mountDirName/$file_idx#", "$mountDirName/d$tag_to_add/$file_idx";
        }
    }
}

sub getopt
{
    my @res = ();
    if (scalar(@ARGV) > 0)
    {
        for (my $i = 0; $i < scalar(@ARGV); $i += 1)
        {
            my $f = $ARGV[$i];
            if ($f eq "--data-dir")
            {
                $dataDirName = $ARGV[$i+1];
                $i += 1;
            }
            elsif ($f eq "--keep-data-dir")
            {
                print "keeping data dir\n";
                $keepDataDir = 1;
            }
            else
            {
                push(@res, $f);
            }
        }
    }
    @res;
}

sub main
{
    my @args = getopt();
    eval {
        setup();
        generate();
    };
    teardown();
}

main()
