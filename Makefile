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
default: bikhitron

over: clean default

NETDYNLIB = $(NETDYNDIR)/libnet-dyn.a

$(NETDYNLIB) : /proc/uptime
	$(MAKE) -C $(NETDYNDIR)

SIMOBJS = bikhitron.o
bikhitron : $(SIMOBJS) $(NETDYNLIB)
	$(CXX) $(CFLAGS) $(SIMOBJS) $(LDFLAGS) -o $@

# to make animation from bikhitron microstate data
%.out/microstate.000000.frame.png : %.out/microstate.csv $(BIKHDIR)/lattice-animation.py
	python $(BIKHDIR)/lattice-animation.py $*.out/microstate.csv

%.out/microstate.animation.ogv : %.out/microstate.animation.mpg
	ffmpeg -i $< -vcodec libtheora $@

%.out/microstate.animation.mpg : %.out/microstate.000000.frame.png
	mencoder mf://$*.out/microstate.*.frame.png -mf type=png:w=800:h=400:fps=20 -ovc lavc -lavcopts vcodec=mpeg4 -oac copy -o $*.out/microstate.animation.mpg

%.out/microstate.animation.gif : %.out/microstate.000000.frame.png
	convert -adjoin -delay 10 $*.out/microstate.*.frame.png -delay 700 `echo $*.out/microstate.*.frame.png | sed 's/.* //'` $@

#	$(RM) $*/microstate.*.frame.png

%.out/microstate.csv : %.settings $(BIKHDIR)/bikhitron
	$(BIKHDIR)/bikhitron --outputDirectory=$*.out -f $<
	$(RM) $*.out/*.frame.png

.PRECIOUS: %.out %.out/microstate.csv %.out/microstate.000000.frame.png %.out/microstate.animation.gif %.out/microstate.animation.mpg %.out/microstate.animation.ogv

# batch simulations

batch-data/%/summary.csv : batch.pl bikhitron
	./batch.pl

batch-data/summaries.csv :
	sed -n -e 1p -e /^0/p batch-data/*/*/*/summary.csv > $@

batch-data/summaries.png : batch-data/summaries.csv
	python batch-plots.py $<

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
