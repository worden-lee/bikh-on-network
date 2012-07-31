#!/usr/bin/perl
use File::Path;
use File::Spec::Functions qw(rel2abs);
use File::Basename;

my $reps = 100;
my @prange = (0.5,1);
my $pstep = 0.01;

my @plist =
 map { $_ * $pstep } (($prange[0]/$pstep) .. ($prange[1]/$pstep));

my $pwd = `pwd`;
chomp($pwd);

my $code_dir = dirname(rel2abs($0));

my $experiment = "10x10";
my $batchdir = "$pwd/batch-data";
my $outdir = "$batchdir/$experiment";

if (!-e $outdir)
{ mkpath($outdir) or die "couldn't create $outdir";
#   symlink("$outdir/network","$outdir/network.dot")
#     or die "couldn't create 'network' symbolic link";
}

print "@explist\n";

for my $i (1 .. $reps)
{ for my $p (@plist)
    { my @extra_args = ("p=$p");
      my($catdir) = "$outdir/".join(".",@extra_args);
      my($dest) = "$catdir/out.$i";
      next if (-e $dest);
      if (!-e "out") { mkdir("out") or die "couldn't mkdir out"; }
      system("rm -rf out/*");
      my $comm = "$code_dir/bikhitron -f $code_dir/$experiment.settings ".
        join("", map {" --$_" } @extra_args);
      print "$comm\n";
      system($comm) == 0 or die "error running $comm";
      ## @@ !! double check this is the right file to check for existence !!
      if (-e "out/microstate.csv")
      { if (!-e $catdir) { mkpath($catdir) or die "couldn't create $catdir"; }
        $comm = "cp -r --force --backup=numbered out/ $dest";
        print "$comm\n";
        system($comm) == 0 or die "error running $comm";
      }
    }
}

# touch the main output directory - useful for make rules sometimes
$now = time;
utime $now, $now, $batchdir;