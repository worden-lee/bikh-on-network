#!/usr/bin/perl
use File::Path;
use File::Spec::Functions qw(rel2abs);
use File::Basename;

# ==== options ====

# set default batch name
my $batchname = 'lattice';

# and default parameters
# if batch name is set in argv, it will customize these values

#my $reps = 1000;
my $reps = 320 / 4;

my @prange = (0.5,1);
my $pstep = 0.01;
#my @nblist = (4,8,12);
my @nblist = (4,12);
#my @rulelist = ("counting", "bayesian");
my @rulelist = ('bayesian-closure-2', 'bayesian-closure-3', 'bayesian-closure-4', 'bayesian-closure-10', 'bayesian-same-neighborhood', 'bayesian');
my @experiments = ("50x50");

# batchnumber is only used if explicitly given.
# It's to run multiple copies of batch in parallel.
my $batchnumber = undef;

# boolean for keeping track of whether we're simulating on a lattice
my $lattice = 1;

use Getopt::Long;
GetOptions(
	'batchname=s'   => \$batchname,
	'batchnumber=i' => \$batchnumber,
	'quick'         => \$quick,
        'keep'          => \$keep
);

if ($quick)
{ $reps = 1; 
  $pstep = 0.05;
  print "quick\n"; 
}

my $batchargs;

if ($batchname eq 'regular-size' or $batchname eq 'regular-size-100')
{ $batchargs = ' -f regular.settings';
  #@nblist = (2,4,6,8,10,14,17,20,24,27,30,27,30,34);
  @nblist = (2,5,8,11,14,17,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200,210,220,230,240,250,260,270,280,290,300);
  @prange = (0.55, 0.55+$pstep/2);
  $lattice = 0;
  if ($batchname eq 'regular-size-100')
  { @experiments = ("100x100"); 
  }
}
elsif ($batchname eq 'regular')
{ $batchargs = " -f regular.settings"; # --n_vertices=2500";
  @nblist = (4,12,50);
  $lattice = 0;
}
elsif ($batchname eq 'lattice-size' or $batchname eq 'lattice-size-100')
{ #@nblist = (1,2,3,4,5,6,7,8,9,10,11,14,17,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200,210,220,230,240,250,260,270,280,290,300);
  @nblist = (4,8,12,24,40,48,60,80,84,120,112,144,168,180,220,224,288,360,440);
  # note there's a special case in the code to skip larger bayesian neighborhoods
  @prange = (0.55, 0.55+$pstep/2);
  #@rulelist = ('counting');
  if ($batchname eq 'lattice-size-100')
  { @experiments = ("100x100"); 
  }
}
elsif ($batchname eq 'figure1-lattice' or $batchname eq 'figure1-regular')
{ #@nblist = (4,8,12,24,40,48,60,80,84,120,112,144,168,180,220,224,288,360,440);
  # python -c 'print [ (2*n + 1)*(2*n + 1) - 1 for n in range(20) ]'
  @nblist = ( 8, 24, 48, 80, 120, 168, 224, 288, 360, 440, 528, 624, 728, 840, 960, 1088, 1224, 1368, 1520 );
  # temporary: make up inbetween sizes
  @nblist = ( 4, 12, 40, 60, 84, 112, 144, 180, 180, 220 );
  # note there's a special case in the code to skip larger 
  # bayesian neighborhoods
  @rulelist = ('bayesian-same-neighborhood'); #, 'bayesian'); 
  @prange = (0.55, 0.55+$pstep/2);
  @experiments = ('100x100');
  if ($batchname eq 'figure1-regular')
  { $batchargs = ' -f regular.settings';
    $lattice = 0;
  }
}
elsif ($batchname eq 'test')
{ #@nblist = (4,8,12,24,40,48,60,80,84,120,112,144,168,180,220,224,288,360,440);
  @nblist = ( 8, 24, 48, 80, 120, 168 ); #, 224, 288, 360, 440, 528, 624, 728, 840, 960, 1088, 1224, 1368, 1520 );
  @prange = (0.55, 0.55+$pstep/2);
  @rulelist = ('bayesian-same-neighborhood');#,'bayesian', 'bayesian-closure-2','bayesian-closure-3', 'bayesian-closure-1');
  $batchargs = ' -f regular.settings';
  $lattice = 0;
  $reps = 1;
}
elsif ($batchname eq 'lattice')
{ @nblist = (4,12,50); # note special case skipping bayesian/50 combination below
}
else
{ die "Unrecognized batch name '$batchname'";
}
#$batchargs .= " --n_vertices=100";

my @plist =
 map { $_ * $pstep } (($prange[0]/$pstep) .. ($prange[1]/$pstep));

#print 'prange: ' , @prange , ' plist: ' , @plist;

my $pwd = `pwd`;
chomp($pwd);

my $code_dir = dirname(rel2abs($0));

my $batchdir = "$pwd/$batchname-batch";
if ( defined $batchnumber ) {
	$batchdir .= "/$batchnumber";
}

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

my $tmpout = "$batchdir/tmp-out";
for my $i (1 .. $reps)
{ for my $rule (@rulelist)
  { for my $nb (@nblist)
    { my $nr = 0;
      my $metric; 
      for my $c (1 .. 20)
      { my $tnr = $c*$c + ($c+1)*($c+1) - 1;
        my $inr = (2*$c+1)*(2*$c+1) - 1;
        #print STDERR "$tnr, $inr\n";
        if ( $nb == $tnr )
        { $nr = $c; $metric = 'taxicab'; last; }
        elsif ( $nb == $inr )
        { $nr = $c; $metric = 'infinity'; last; }
      }
      if ( $lattice and $nr == 0 ) 
      { die "unaccountable neighborhood size $nb"; }
      if ($lattice and $rule eq "bayesian" and $nb > 50)
      { warn "lattice neighborhood size $nb is too large for full bayesian simulation"; 
        next;
      }
      for my $p (@plist)
      { for my $experiment (@experiments)
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

	  print '' . localtime(time) . "\n";

          if (!-e "$tmpout") { mkdir("$tmpout") or die "couldn't mkdir $tmpout"; }
          system("rm -rf $tmpout/*");
	  if ( $rule =~ /bayesian-closure-(.*)$/ ) {
		  push @extra_args, "inference_closure_level=$1", "update_rule=bayesian-closure";
	  }
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
              #rename("$tmpout/settings.csv", "$batchdir/settings.$seed.csv");
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
