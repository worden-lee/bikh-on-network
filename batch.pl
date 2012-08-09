#!/usr/bin/perl
use File::Path;
use File::Spec::Functions qw(rel2abs);
use File::Basename;

my $reps = 1000;
if (grep(/^--quick$/,@ARGV))
{ $reps = 2; print "quick\n"; }

my @prange = (0.5,1);
my $pstep = 0.01;
my @plist =
 map { $_ * $pstep } (($prange[0]/$pstep) .. ($prange[1]/$pstep));

my @nblist = (4,8);

my @rulelist = ("pluralistic-ignorance", "approximate-inference");

my $pwd = `pwd`;
chomp($pwd);

my $code_dir = dirname(rel2abs($0));

my @experiments = ("50x50");
my $batchdir = "$pwd/batch-data";
my $outdir = "$batchdir";

my $batchcsv = "$batchdir/batch.csv";
open BATCHCSV, ">$batchcsv" or die "opening $batchcsv for writing";
print BATCHCSV "name,experiment,neighbors,p\n";

if (!-e $outdir)
{ mkpath($outdir) or die "couldn't create $outdir";
}

print "@explist\n";

for my $i (1 .. $reps)
{ for my $rule (@rulelist)
	{ for my $p (@plist)
		{ for my $nb (@nblist)
			{ for my $experiment (@experiments)
				{ my @extra_args = ("update-rule=$rule","n_neighbors=$nb","p=$p");
					my @extra_dirs = ($experiment,"update-rule_$rule","n_neighbors_$nb","p_$p");
					my $dirname = join("/",@extra_dirs);
					my($catdir) = "$outdir/".$dirname;
					my($dest) = "$catdir/out.$i";
					print BATCHCSV "$dirname,$experiment,$nb,$p\n";
					next if (-e $dest);
					if (!-e "out") { mkdir("out") or die "couldn't mkdir out"; }
					system("rm -rf out/*");
					my $comm = "$code_dir/bikhitron -f $code_dir/settings/$experiment.settings ".
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
		}
	}
}

# touch the main output directory - useful for make rules sometimes
$now = time;
utime $now, $now, $batchdir;
