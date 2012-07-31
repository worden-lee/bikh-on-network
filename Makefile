NETWERK ?= ../../netwerk
NETDYNDIR ?= $(NETWERK)/net-dyn-lib
VXLDIR ?= $(NETWERK)/vxl
ESTRDIR ?= $(NETWERK)/libexecstream
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
%.out/microstate.000000.frame.png : %.out/microstate.csv lattice-animation.py
	python -m memory_profiler lattice-animation.py $*.out/microstate.csv

%.out/microstate.animation.gif : %.out/microstate.000000.frame.png
	convert -adjoin -delay 10 $*.out/microstate.*.frame.png -delay 700 `echo $*.out/microstate.*.frame.png | sed 's/.* //'` $@

#	$(RM) $*/microstate.*.frame.png

%.out/microstate.csv : %.settings ./bikhitron
	./bikhitron --outputDirectory=$*.out -f $<

.PRECIOUS: %.out %.out/microstate.000000.frame.png %.out/microstate.csv %.out/microstate.animation.gif

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
