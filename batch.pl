#!/usr/bin/perl
use File::Path;
use File::Spec::Functions qw(rel2abs);
use File::Basename;

#my $reps = 1000;
my $reps = 1;

my @prange = (0.5,1);
my $pstep = 0.01;
#my @nblist = (4,8,12);
my @nblist = (4,12);
my @rulelist = ("counting", "bayesian");
my @experiments = ("50x50");

if (grep(/^--quick$/,@ARGV))
{ $reps = 1; 
	$pstep = 0.05;
	print "quick\n"; 
}
if (grep(/^--keep$/,@ARGV)) # this is not so well tested
{ $keep = 1; print "keep\n"; }

my($batchname, $batchargs);
if (grep(/^--regular-size$/,@ARGV))
{ $batchname = "regular-size";
  $batchargs = ' -f regular.settings';
  #@nblist = (2,4,6,8,10,14,17,20,24,27,30,27,30,34);
  @nblist = (2,5,10,15,20,25,30,35,40,45,50,55,60);
  @prange = (0.55, 0.55+$pstep/2);
  @experiments = ("50x50","100x100");
}
elsif (grep(/^--regular$/,@ARGV))
{ $batchname = "regular"; 
  $batchargs = " -f regular.settings"; # --n_vertices=2500";
  @nblist = (4,12,50);
}
elsif (grep(/^--lattice-size$/,@ARGV))
{ $batchname = "lattice-size";
  @nblist = (2,5,10,15,20,25,30,35,40,45,50,55,60);
  @prange = (0.55, 0.55+$pstep/2);
  @experiments = ("50x50","100x100");
}
else # if (grep(/^--lattice$/,@ARGV))
{ $batchname = "lattice";
  @nblist = (4,12,50); # note special case skipping bayesian/50 combination below
}
#$batchargs .= " --n_vertices=100";

my @plist =
 map { $_ * $pstep } (($prange[0]/$pstep) .. ($prange[1]/$pstep));

my $pwd = `pwd`;
chomp($pwd);

my $code_dir = dirname(rel2abs($0));

my $batchdir = "$pwd/$batchname-batch";

my $outdir = $batchdir;

if (!-e $outdir)
{ mkpath($outdir) or die "couldn't create $outdir";
}

my $batchcsv = "$batchdir/batch.csv";
if (-e $batchcsv) {
	open BATCHCSV, ">>$batchcsv" or die "opening $batchcsv for appending";
} else {
	open BATCHCSV, ">$batchcsv" or die "opening $batchcsv for writing";
	print BATCHCSV "name,experiment,update rule,neighbors,p\n";
}

print "@explist\n";

my $tmpout = "$batchdir/tmp-out";
for my $i (1 .. $reps)
{ for my $rule (@rulelist)
	{ for my $p (@plist)
		{ for my $nb (@nblist)
			{ my $nr; if ($nb == 12) { $nr = 2; } else { $nr = 1; }
				my $metric; 
				if ($nb == 8) { $metric = "infinity"; } else { $metric = "taxicab"; }
				if ($batchname eq "lattice" and $rule eq "bayesian" and $nb == 50) { next; }
				for my $experiment (@experiments)
				{ my @extra_args = ("update_rule=$rule","neighborhood_radius=$nr",
						"lattice_metric=$metric","n_neighbors=$nb","p=$p","rep=$i");
					my @extra_dirs = ($experiment,"update_rule_$rule","n_neighbors_$nb","p_$p");
					my $dirname = join("/",@extra_dirs);
					my($catdir, $dest);
					if ($keep) {
						$catdir = "$outdir/".$dirname;
						$dest = "$catdir/out.$i";
						print BATCHCSV "$dirname,$experiment,$rule,$nb,$p\n";
						next if (-e $dest);
					}
					if (!-e "$tmpout") { mkdir("$tmpout") or die "couldn't mkdir $tmpout"; }
					system("rm -rf $tmpout/*");
					my $comm = "$code_dir/bikhitron "
						. " -f $code_dir/settings/$experiment.settings "
						. $batchargs . join("", map {" --$_" } @extra_args)
						. " --outputDirectory=$tmpout";
					print "$comm\n";
					system($comm) == 0 or die "error running $comm";
					## @@ !! double check this is the right file to check for existence !!
					if (-e "$tmpout/settings.csv")
					{ if ($keep)
						{ if (!-e $catdir) 
							{ mkpath($catdir) or die "couldn't create $catdir"; }
							$comm = "cp -r --force --backup=numbered $tmpout/ $dest";
							print "$comm\n";
							system($comm) == 0 or die "error running $comm";
						}
						else 
						{ $seed = `grep randSeed $tmpout/settings.csv | sed s/randSeed,//`;
							chomp $seed;
							print BATCHCSV "$seed,$experiment,$rule,$nb,$p\n";
							rename("$tmpout/settings.csv", "$batchdir/settings.$seed.csv");
							if (-e "$batchdir/summaries.csv") {
								system("sed -n -e 2p $tmpout/summary.csv >> $batchdir/summaries.csv");
							} else {
								print "OVERWRITE summaries.csv\n";
								rename("$tmpout/summary.csv", "$batchdir/summaries.csv");
							}
						}
					}
				}
			}
		}
	}
}

# touch the main output directory - useful for make rules sometimes
$now = time;
utime $now, $now, $batchdir;
