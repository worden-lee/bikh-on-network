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
default: simulation

over: clean default

SRCS = simulation.cpp
_OINT = $(SRCS:.cpp=.o)
OBJS = $(_OINT:.c=.o)

NETDYNLIB = $(NETDYNDIR)/libnet-dyn.a

$(NETDYNLIB) : /proc/uptime
	$(MAKE) -C $(NETDYNDIR)

SIMOBJ = simulation.o
simulation : $(SIMOBJ) $(NETDYNLIB)
	$(CXX) $(CFLAGS) $< $(LDFLAGS) -o $@

# fancy GNU-style line for tracking header dependencies in .P files
%.o : %.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -save-temps -MD -c $< -o $@
	@cp $*.d $*.P; \
	    sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	        -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	    rm -f $*.d $*.s $*.ii

DEPS = $(OBJS:.o=.P)

-include $(DEPS)

clean:
	$(RM) *.o *.P *~ *#

clear:
	$(RM) -r out

%.eps : %.dot
	neato -Goverlap=scale -Gsplines=true -Tps -o $@ $<

# when network.dot is changed to network~2~
%~.eps : %~
	neato -Goverlap=scale -Gsplines=true -Tps -o $@ $<
