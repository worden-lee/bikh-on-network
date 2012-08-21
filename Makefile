# directories to link to
NETWERK ?= ../../netwerk
NETDYNDIR ?= $(NETWERK)/net-dyn-lib
VXLDIR ?= $(NETWERK)/vxl
ESTRDIR ?= $(NETWERK)/libexecstream
# this directory
BIKHDIR ?= .
# add -pg for profiling
CFLAGS=-g -I$(NETDYNDIR) -I$(VXLDIR)/core -I$(VXLDIR)/vcl -I$(VXLDIR)/core/vnl -I$(VXLDIR)/v3p/netlib -I$(VXLDIR)/v4p/netlib -I$(ESTRDIR)
LDFLAGS=-L$(NETDYNDIR) -L$(VXLDIR)/core/vnl/algo -L$(VXLDIR)/core/vnl/ -L$(VXLDIR)/vcl -L$(VXLDIR)/v3p/netlib -L$(VXLDIR)/lib $(ESTRDIR)/exec-stream.o -lnet-dyn -lvnl_algo -lvnl -lvcl -lnetlib -lv3p_netlib -lpthread

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
	$(MAKE) -C $(NETDYNDIR)

SIMOBJS = $(BIKHDIR)/bikhitron.o
$(BIKHDIR)/bikhitron : $(SIMOBJS) $(NETDYNLIB)
	$(CXX) $(CFLAGS) $(SIMOBJS) $(LDFLAGS) -o $@

# run the simulation
%.out/summary.csv %.out/microstate.csv : $(BIKHDIR)/settings/%.settings $(BIKHDIR)/bikhitron $(BIKHDIR)/settings/defaults-cascade.settings
	$(BIKHDIR)/bikhitron --outputDirectory=$*.out -f $<
	$(RM) $*.out/*.frame.png
	
# make animation from bikhitron microstate data
%/microstate.000000.frame.png : %/microstate.csv $(BIKHDIR)/lattice-animation.py
	python $(BIKHDIR)/lattice-animation.py $<

%/microstate.animation.ogv : %/microstate.animation.mpg
	ffmpeg -i $< -y -vcodec libtheora $@

%/microstate.animation.mpg : %/microstate.000000.frame.png
	mencoder mf://$*/microstate.*.frame.png -mf type=png:w=800:h=400:fps=20 -ovc lavc -lavcopts vcodec=mpeg4 -oac copy -o $@

%/microstate.animation.gif : %/microstate.000000.frame.png
	convert -adjoin -delay 10 $*/microstate.*.frame.png -delay 700 `echo $*.out/microstate.*.frame.png | sed 's/.* //'` $@

%.out :
	mkdir $@

.PRECIOUS: $(BIKHDIR)/bikhitron %.out %.out/summary.csv %.out/microstate.csv %.out/microstate.animation.gif %.out/microstate.animation.mpg %.out/microstate.animation.ogv

# batch simulations

lattice-batch/batch.csv lattice-batch/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --lattice

#lattice-batch/summaries.csv : lattice-batch/batch.csv
#	sed -n -e 1p -e /^0/p `find lattice-batch -name summary.csv` > $@

regular-batch/batch.csv regular-batch/summaries.csv : $(BIKHDIR)/batch.pl $(BIKHDIR)/bikhitron
	$(BIKHDIR)/batch.pl --regular

%/summaries.mean.png %/summaries.probability.png %/summaries.last.png %/summaries.size-last.png %/summaries.size-mean.png : %/summaries.csv $(BIKHDIR)/batch-plots.py
	python $(BIKHDIR)/batch-plots.py $<

batch-data :
	mkdir $@

.PRECIOUS: lattice-batch lattice-batch/batch.csv lattice-batch/summaries.csv regular-batch lattice-batch/batch.csv regular-batch/summaries.csv

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
