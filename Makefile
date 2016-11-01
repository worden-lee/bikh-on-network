# directories to link with
NETWERK = ../worden-lee_net-dyn
NETDYNDIR = $(NETWERK)/net-dyn-lib
VXLDIR ?= $(NETWERK)/vxl
$(warning NETDYN $(NETDYN))
$(warning NETWERK $(NETWERK))
$(warning realpath NETWERK $(realpath $(NETWERK)))
$(warning in .. $(shell ls ..))
ESTRDIR = $(realpath $(NETWERK)/libexecstream)
$(warning ESTRDIR $(ESTRDIR))
# this directory
BIKHDIR ?= .
# add -pg for profiling
CFLAGS=-g -I$(NETDYNDIR) -I$(VXLDIR)/core -I$(VXLDIR)/vcl -I$(VXLDIR)/core/vnl -I$(VXLDIR)/v3p/netlib -I$(VXLDIR)/v4p/netlib -I$(ESTRDIR)# -pg
LDFLAGS=-L$(NETDYNDIR) -L$(VXLDIR)/core/vnl/algo -L$(VXLDIR)/core/vnl/ -L$(VXLDIR)/vcl -L$(VXLDIR)/v3p/netlib -L$(VXLDIR)/lib -lnet-dyn -lvnl_algo -lvnl -lvcl -lnetlib -lv3p_netlib -lpthread

# enable to use compiler optimization
OPTIMIZE?=yes
ifeq ($(OPTIMIZE),yes)
CFLAGS+=-O3
endif

#default: swarming
default: $(BIKHDIR)/bikhitron

over: clean default

NETDYNLIB = $(NETDYNDIR)/libnet-dyn.a

$(NETDYNLIB) : /proc/uptime
	$(MAKE) -C $(NETDYNDIR) $(subst $(NETDYNDIR)/,,$(NETDYNLIB))

$(ESTRDIR)/exec-stream.o : $(ESTRDIR)/exec-stream.cpp $(ESTRDIR)/exec-stream.h
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

SIMOBJS = $(BIKHDIR)/bikhitron.o $(ESTRDIR)/exec-stream.o 

$(BIKHDIR)/bikhitron : $(SIMOBJS) $(NETDYNLIB)
	$(CXX) $(CFLAGS) $(SIMOBJS) $(LDFLAGS) -o $@

# run the simulation
%.out/summary.csv %.out/microstate.csv : $(BIKHDIR)/settings/%.settings $(BIKHDIR)/bikhitron $(BIKHDIR)/settings/defaults-cascade.settings
	$(BIKHDIR)/bikhitron --outputDirectory=$*.out -f $<
	$(RM) $*.out/*.frame.png
	
# make animation from bikhitron microstate data
%/cumulative-mean-density.png %/microstate.000000.frame.png %/microstate.002500.frame.png : %/microstate.csv $(BIKHDIR)/microstate-plots.py
	python $(BIKHDIR)/microstate-plots.py $<

%/microstate.animation.ogv : %/microstate.animation.mpg
	ffmpeg -i $< -y -vcodec libtheora $@

%/microstate.animation.mpg : %/microstate.000000.frame.png
	mencoder mf://$*/microstate.*.frame.png -mf type=png:w=800:h=400:fps=20 -ovc lavc -lavcopts vcodec=mpeg4 -oac copy -o $@

%/microstate.animation.gif : %/microstate.000000.frame.png
	convert -adjoin -delay 10 $*/microstate.*.frame.png -delay 700 `echo $*.out/microstate.*.frame.png | sed 's/.* //'` $@

# use frames from the animation as lattice pictures

# make network picture from simulation output
%/network.png : %/network.csv ./plot-network.py
	python ./plot-network.py $< $@

%.out :
	mkdir $@

.PRECIOUS: $(BIKHDIR)/bikhitron %.out %.out/summary.csv %.out/microstate.csv %.out/microstate.animation.gif %.out/microstate.animation.mpg %.out/microstate.animation.ogv

# batch simulations

test-batch/batch.csv test-batch/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname test

lattice-batch/batch.csv lattice-batch/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname lattice

#lattice-batch/summaries.csv : lattice-batch/batch.csv
#	sed -n -e 1p -e /^0/p `find lattice-batch -name summary.csv` > $@

regular-batch/batch.csv regular-batch/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname regular

lattice-size-batch/batch.csv lattice-size-batch/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname lattice-size

regular-size-batch/batch.csv regular-size-batch/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname regular-size

lattice-size-100-batch/batch.csv lattice-size-100-batch/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname lattice-size-100

regular-size-100-batch/batch.csv regular-size-100-batch/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname regular-size-100

# Cascades paper batches we do differently: in parallel

figure1-lattice-batch/batch.csv figure1-lattice-batch/summaries.csv : # /proc/uptime
	-(head -n 1 figure1-lattice-batch/1/batch.csv && tail -n +2 -q figure1-lattice-batch/*/batch.csv) > figure1-lattice-batch/batch.csv
	(head -n 1 figure1-lattice-batch/1/summaries.csv && tail -n +2 -q figure1-lattice-batch/*/summaries.csv) > figure1-lattice-batch/summaries.csv

figure1-lattice-batch/%/batch.csv figure1-lattice-batch/%/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname figure1-lattice --batchnumber $*

figure1-regular-batch/batch.csv figure1-regular-batch/summaries.csv : /proc/uptime
	-(head -n 1 figure1-regular-batch/1/batch.csv && tail -n +2 -q figure1-regular-batch/*/batch.csv) > figure1-regular-batch/batch.csv
	(head -n 1 figure1-regular-batch/1/summaries.csv && tail -n +2 -q figure1-regular-batch/*/summaries.csv) > figure1-regular-batch/summaries.csv

figure1-regular-batch/%/batch.csv figure1-regular-batch/%/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname figure1-regular --batchnumber $*

compare-bayesian-batch/batch.csv compare-bayesian-batch/summaries.csv : /proc/uptime
	-(head -n 1 compare-bayesian-batch/1/batch.csv && tail -n +2 -q compare-bayesian-batch/*/batch.csv) > compare-bayesian-batch/batch.csv
	(head -n 1 compare-bayesian-batch/1/summaries.csv && tail -n +2 -q compare-bayesian-batch/*/summaries.csv) > compare-bayesian-batch/summaries.csv

compare-bayesian-batch/%/batch.csv compare-bayesian-batch/%/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --batchname compare-bayesian --batchnumber $*

%/summaries.mean.png %/summaries.probability.png %/summaries.last.png %/summaries.size-last.png %/summaries.size-mean.png %/summaries.size-probability.png %/summaries.frequencies.csv : %/summaries.csv $(BIKHDIR)/batch-plots.py
	python $(BIKHDIR)/batch-plots.py $<

batch-data :
	mkdir $@

%-batch :
	mkdir $@

.PRECIOUS: lattice-batch lattice-batch/batch.csv lattice-batch/summaries.csv regular-batch regular-size-batch lattice-batch/batch.csv regular-batch/summaries.csv regular-size-batch/summaries.csv regular-size-100-batch/summaries.csv figure1-regular-batch figure1-regular-batch/summaries.csv figure1-lattice-batch figure1-lattice-batch/summaries.csv

.PRECIOUS: figure1-lattice-batch/1 figure1-lattice-batch/2 figure1-lattice-batch/3 figure1-lattice-batch/4 figure1-lattice-batch/5 figure1-lattice-batch/6 figure1-lattice-batch/1/summaries.csv figure1-lattice-batch/2/summaries.csv figure1-lattice-batch/3/summaries.csv figure1-lattice-batch/4/summaries.csv figure1-lattice-batch/5/summaries.csv figure1-lattice-batch/6/summaries.csv

.PRECIOUS: figure1-regular-batch/1 figure1-regular-batch/2 figure1-regular-batch/3 figure1-regular-batch/4 figure1-regular-batch/5 figure1-regular-batch/6 figure1-regular-batch/1/summaries.csv figure1-regular-batch/2/summaries.csv figure1-regular-batch/3/summaries.csv figure1-regular-batch/4/summaries.csv figure1-regular-batch/5/summaries.csv figure1-regular-batch/6/summaries.csv

# fancy GNU-style line for tracking header dependencies in .P files
%.o : %.c++
	$(CXX) $(CFLAGS) $(CXXFLAGS) -save-temps -MD -c $< -o $@
	@cp $*.d $*.P; \
	    sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	        -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	    rm -f $*.d $*.s $*.ii

OBJS = $(SIMOBJS)
DEPS = $(OBJS:.o=.P)

-include $(DEPS)

clean:
	$(RM) *.o *.P *~ *#

clear:
	$(RM) -r out

%.eps : %.dot
	neato -Goverlap=scale -Gsplines=true -Tps -o $@ $<

%.open : %
	open $<
.PHONY : %.open
