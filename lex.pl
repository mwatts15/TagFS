#!/usr/bin/env perl
use strict;
use warnings;
use File::Basename;

my $file = shift;
my %format_specs =
( "const char *" => "\"%s\""
    , "__DNP__ char *" => "%p"
    , "__DNP__ const char *" => "%p"
    , "char *" => "\"%s\""
    , "long" => "%ld"
    , "size_t" => "%zd"
    , "off_t" => "%lld"
    , "strut fuse_file_info *" => "0x%08x"
    , "int" => "%d"
    , "uid_t" => "%d"
    , "gid_t" => "%d"
    , "mode_t" => "0%03o"
    , "dev_t" => "%lld"
);

sub c_str_esc
{
    $_ = shift;
    s/"/\\"/g;
    $_;
}

sub print_file {
    my $outfile = $file;
    $outfile =~ s/\..+$/.c/;
    open( my $fh, ">$outfile" ) or die "can't create $outfile $!";
    print $fh @_;
}

local( *FH ) ;
open( FH, $file ) or die "Cannot open $file: $!";
my $F = do { local( $/ ) ; <FH> };
{ #function headers and component structs
    #turns %%operation <argument names>%% into
    #int <file_name_base>_<operation> (<arguments>)

    my %oper_headers =
    ( getattr => " int %sgetattr (const char *%s, struct stat *%s)"
        , readlink => " int %sreadlink (const char *%s, __DNP__ char *%s, size_t %s)"
        , getdir => " int %sgetdir (const char *%s, fuse_dirh_t %s, fuse_dirfil_t %s)"
        , mknod => " int %smknod (const char *%s, mode_t %s, dev_t %s)"
        , mkdir => " int %smkdir (const char *%s, mode_t %s)"
        , unlink => " int %sunlink (const char *%s)"
        , rmdir => " int %srmdir (const char *%s)"
        , symlink => " int %ssymlink (const char *%s, const char *%s)"
        , rename => " int %srename (const char *%s, const char *%s)"
        , link => " int %slink (const char *%s, const char *%s)"
        , chmod => " int %schmod (const char *%s, mode_t %s)"
        , chown => " int %schown (const char *%s, uid_t %s, gid_t %s)"
        , truncate => " int %struncate (const char *%s, off_t %s)"
        , utime => " int %sutime (const char *%s, struct utimbuf *%s)"
        , open => " int %sopen (const char *%s, struct fuse_file_info *%s)"
        , read => " int %sread (const char *%s, __DNP__ char *%s, size_t %s, off_t %s, struct fuse_file_info *%s)"
        , write => " int %swrite (const char *%s, __DNP__ const char *%s, size_t %s, off_t %s, struct fuse_file_info *%s)"
        , statfs => " int %sstatfs (const char *%s, struct statvfs *%s)"
        , flush => " int %sflush (const char *%s, struct fuse_file_info *%s)"
        , release => " int %srelease (const char *%s, struct fuse_file_info *%s)"
        , fsync => " int %sfsync (const char *%s, int %s, struct fuse_file_info *%s)"
        , setxattr => " int %ssetxattr (const char *%s, const char *%s, const char *%s, size_t %s, int %s)"
        , getxattr => " int %sgetxattr (const char *%s, const char *%s, char *%s, size_t %s)"
        , listxattr => " int %slistxattr (const char *%s, char *%s, size_t %s)"
        , removexattr => " int %sremovexattr (const char *%s, const char *%s)"
        , opendir => " int %sopendir (const char *%s, struct fuse_file_info *%s)"
        , readdir => " int %sreaddir (const char *%s, void *%s, fuse_fill_dir_t %s, off_t %s, struct fuse_file_info *%s)"
        , releasedir => " int %sreleasedir (const char *%s, struct fuse_file_info *%s)"
        , fsyncdir => " int %sfsyncdir (const char *%s, int %s, struct fuse_file_info *%s)"
        , destroy => " void %sdestroy (void *%s)"
        , access => " int %saccess (const char *%s, int %s)"
        , create => " int %screate (const char *%s, mode_t %s, struct fuse_file_info *%s)"
        , ftruncate => " int %sftruncate (const char *%s, off_t %s, struct fuse_file_info *%s)"
        , fgetattr => " int %sfgetattr (const char *%s, struct stat *%s, struct fuse_file_info *%s)"
        , lock => " int %slock (const char *%s, struct fuse_file_info *%s, int %s, struct flock *%s)"
        , utimens => " int %sutimens (const char *%s, const struct timespec %s[2])"
    );
    my @oper_names = keys(%oper_headers);

    sub make_fuse_oper 
    {
        my ($base, @ops) = @_;
        return "struct fuse_operations ${base}oper = {"
        . join (",", map {sprintf(".%s = ${base}%s", $_, $_)} @ops)
        . "};";
    }

    my $op_alt = join("|", @oper_names);
    my @these_opers = ();

    my ($base, undef) = fileparse($file, qr/\.[^.]*/);
    $base = $base . "_";

    $F =~ s<^\s*op%($op_alt) (.*)%><push(@these_opers, $1); sprintf($oper_headers{$1}, $base, split(" ", $2))>mge;
    $F =~ s<%%%fuse_operations%%%><&make_fuse_oper($base, @these_opers)>mge;
}

{ #logging tagfs operations
    sub add_fn_log_msg
    {
        my ($name, $args) = @_;
        #print "$orig\n$name\n$args\n$rest\n";
        my @arg_list = split /, ?/, $args;
        my @arg_formats = ();
        my @arg_names = ();
        foreach (@arg_list)
        {
            # get the type and argument
            if (m/(.*)\b(\w+)$/)
            {
                my $type = $1;
                my $arg = $2;
                my $format = (defined $format_specs{$type})?
                $format_specs{$type} : "%p";
                push @arg_formats, "$arg=" . c_str_esc($format);
                push @arg_names, $arg;
            }
        }
        "log_msg(\"\\n$name " . join (" ", @arg_formats) . "\", " . 
        join(", ", @arg_names) . ");";
    }
    $F =~ s<^(
    \s* # whitespace
    \w+ # the function return type
    \s*
    (\w+) # the function name 
    \s* #ws
    \(
    ([^\)]*) #arguments
    \)
    \s*
    )
    %log% # log notice
    (\s*\{) #everything else
    ><"$1$4" . &add_fn_log_msg($2, $3)>mgex;
}
{ #queries
    # HEADERS
    # results
    # structs
}
&print_file($F);
