#!/usr/bin/perl
use File::Path;
use File::Spec::Functions qw(rel2abs);
use File::Basename;

my $reps = 10000;
my @exprange = (1.1,3.0);
my $expstep = 0.1;

my @explist =
 map { $_ * $expstep } (($exprange[0]/$expstep) .. ($exprange[1]/$expstep));

my $pwd = `pwd`;
chomp($pwd);

my $code_dir = dirname(rel2abs($0));

my $experiment = "network-power-sample";
my $batchdir = "$pwd/batch-data";
my $outdir = "$batchdir/$experiment";

if (!-e $outdir)
{ mkpath($outdir) or die "couldn't create $outdir";
#   symlink("$outdir/network","$outdir/network.dot")
#     or die "couldn't create 'network' symbolic link";
}

print "@explist\n";

for my $i (1 .. $reps)
{ for my $inexp (@explist)
  { for my $outexp (@explist)
    { my @extra_args = ("pl_in_exp=$inexp","pl_out_exp=$outexp",
                        "mutantFitness=1.05");
      my($catdir) = "$outdir/".join(".",@extra_args);
      my($dest) = "$catdir/out.$i";
      next if (-e $dest);
      if (!-e "out") { mkdir("out") or die "couldn't mkdir out"; }
      system("rm -rf out/*");
      my $comm = "$code_dir/network-sample -f $code_dir/settings/$experiment.settings ".
        join("", map {" --$_" } @extra_args);
      print "$comm\n";
      system($comm) == 0 or die "error running $comm";
      ## @@ !! double check this is the right file to check for existence !!
      if (-e "out/network.csv")
      { if (!-e $catdir) { mkpath($catdir) or die "couldn't create $catdir"; }
        $comm = "cp -r --force --backup=numbered out/ $dest";
        print "$comm\n";
        system($comm) == 0 or die "error running $comm";
      }
    }
  }
}

# touch the main output directory - useful for make rules sometimes
$now = time;
utime $now, $now, $batchdir;
