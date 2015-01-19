#!/usr/bin/env perl

# This script generates a (presumably) correctly 
# structured test database by using an (allegedly) 
# correct tagfs. 
#
# It should only be used once all of the other tests
# which don't use the test database pass.

my $mountDirName;
my $dataDirName;
my $DATABASE = "test.db";
my $MAX_FILES = 10;
my $MAX_TAGS = 5;
my $MAX_TAGS_PER_FILE = 3;

($MAX_TAGS_PER_FILE < $MAX_TAGS) || die "You can't have more tags per file than you have tags.";

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
    $mountDirName = make_tempdir("mount");
    $dataDirName = make_tempdir("data");
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
    `rm -rf $dataDirName`;
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
    for (my $tag_idx = 0; $tag_idx < $MAX_TAGS; $tag_idx++)
    {
        my $dir = "$mountDirName/d$tag_idx";
        if (not (mkdir($dir)))
        {
            print STDERR "Couldn't create $dir. Exiting.";
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
setup();
generate();
END {
    teardown();
}
