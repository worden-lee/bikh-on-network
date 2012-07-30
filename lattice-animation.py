import matplotlib
matplotlib.use('agg')
from pylab import *
import scipy

from scipy import special

# import numpy as np

### Packages which are necessary to create a movie
 
import subprocess                 # For issuing commands to the OS.
import os, fnmatch
import sys                        # For determining the Python version.

if (len(sys.argv) >= 2):
	filenameslist = ["simulation.%s.out" % sys.argv[1]]
else:
  #filenameslist = fnmatch.filter(os.listdir("."),"tsp*sm*mu*85.out")
	filenameslist = fnmatch.filter(os.listdir("."),"simulation.*.out")
filenameslist.sort()

print filenameslist

number_of_txtfiles = len(filenameslist)
 
# Binomial coefficient (taken from wikipedia)
def binomial(n, k):
    if k>n-k: # take advantage of symmetry
        k=n-k
    c = 1.0
    for i in range(k):
        c = c*(n-i)
        c = c/(i+1)
    return c

def F(x,mu,sigma):
    return 0.5+0.5*special.erf((x-mu)/((2.0**0.5)*sigma))

def Fhat(x,mu,sigma,N):
    result = 0.0
    for n in range(0,N+1):
        result += 1.*binomial(N,n)*F(1.*n/N,mu,sigma)*(x**n)*((1.-x)**(N-n))
    return result

def mean(x):
	sum = 0
	for item in x:
		sum = sum + item;
	return 1.0*sum/len(x)

N = 8; mu = 0.55; sigma = 0.225
F_hat = lambda xi : Fhat(xi,mu,sigma,N)
F_hatDiff = lambda xi : Fhat(xi,mu,sigma,N)-xi

mixProb = []; slope = []; minimum_plus = [];

# fig=figure(figsize=(8.5,4))
size = ["100x100","400x400","800x800"]
for i in range(number_of_txtfiles):
	fig=figure(figsize=(4.25,2))
	txt_file = filenameslist[i]
	img_file = txt_file.rstrip(".out")+".dydt.png"
	obj = [line.split() for line in open(txt_file,'r')][0][0]
	obj = obj.lstrip("{").rstrip("}").split("}{")
	x = []; eta = []; y = []; r = []; s = []; pair = [];
	for j in range(len(obj)):
		#print obj[j]
	 	subitem = map(float,obj[j].split(","))
	 	x.append(subitem[0]); y.append(subitem[1]); eta.append(subitem[1]-subitem[0]); 
		s.append(subitem[2]); r.append(subitem[3]); pair.append(subitem[4]);
	ax = fig.add_subplot(1,1,1)
	grid(True); ax.xaxis.set_major_locator(MaxNLocator(4)); ax.yaxis.set_major_locator(MaxNLocator(4))
	# plot(x,eta,marker='.',color='r',ls='',alpha=0.05,zorder=0)
	xx = linspace(0,1,100)
	yy = F_hatDiff(xx)
	plot(xx,yy,'b--',alpha=1.0,zorder=1)
	# plot([0,1.0],[0,0],'k:',zorder=-1)
	xlim(xmin=0,xmax=1.0); ylim(ymin=-.16,ymax=.08);
	# xlim(xmin=0.65,xmax=.72); ylim(ymin=-.025,ymax=.025);
	xlabel("$y(t)$");  ylabel('$\mathrm{d}y/\mathrm{d}t$')
	# To remove the "tail" from the intermediate point 
	# x1 and eta1 belong to the SM approx
	x1 = []; eta1 = [];
	x2 = []; eta2 = [];
	for j in range(len(obj)):
		if (x[j]<.8 and eta[j]<0.001) or (x[j]>.8) or (x[j]<.6):
			x1.append(x[j]); eta1.append(eta[j])
		else:
			x2.append(x[j]); eta2.append(eta[j])
	#plot(x1,eta1,marker='.',color='c',ls='',alpha=0.09,zorder=0)
	#plot(x2,eta2,marker='.',color='g',ls='',alpha=0.01,zorder=0)
	if (1) : # just plot the points
		plot(x,eta,'k.',zorder=2)
		# and plot (c-y) vs y as well
		#plot(x,[ct-yt for yt,ct in zip(x,y)],'r.',zorder=1)
		# plot r
		#plot(x, [ct-rt for ct,rt in zip(y,r)], 'r.')
		# plot s
		#plot(x, [1-st for ct,st in zip(y,s)], 'b.')
		# plot churn
		#plot(x, [2*(yt-rt) for yt,rt, in zip(x,r)], 'g.')
		#plot(eta,pair,'r.')
	else:
		z = np.polyfit(x1,eta1,N+4)
		p = np.poly1d(z)
		xx = np.linspace(0.,1.,100)
		plot(xx,[p(xx1) for xx1 in xx],'r--',zorder=5,linewidth=1.5)
		z = np.polyfit(x2,eta2,2)
		p = np.poly1d(z)
		xx = np.linspace(0.76,.7876,20)
		plot(xx,[p(xx1) for xx1 in xx],'r-',zorder=5,linewidth=0.7)
		plot([0.76],[0.0219864],'ro')
	#text(.04,.05,size[i])
	# text(0.785,-0.016,'$\~y$')
	# text(0.265,-.063,'$\^F_{\~\mu}(y)$')
	# text(0.265,-.11,'$\^F(y)$')
	subplots_adjust(bottom=.14)
	savefig(img_file,format='png')
	# plot activated-site neighborhood density
	fig=figure(figsize=(2,2))
	ax = fig.add_subplot(1,1,1)
	#xlim(xmin=0,xmax=1.0); ylim(ymin=0,ymax=1.0);
	#plot(x,x,'k:')
	xlabel("$y$");  ylabel('$q_1-y$')
	plot(x, [(pt/yt - yt) if yt > 0 else 0 for yt,pt in zip(x,pair)], 'c.')
	yq_file = txt_file.rstrip(".out")+".q-y.png"
	subplots_adjust(bottom=.14,left=.18)
	savefig(yq_file,format='png')
