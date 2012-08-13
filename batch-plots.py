import matplotlib
matplotlib.use('agg')
from pylab import *
import scipy

from scipy import special
import csv

from itertools import groupby

# import numpy as np

### Packages which are necessary to create a movie
 
#import subprocess                 # For issuing commands to the OS.
import os, fnmatch
import sys

if (len(sys.argv) >= 2):
	filenameslist = [sys.argv[1]]
else:
	filenameslist = fnmatch.filter(os.listdir("."),"*.microstate.csv")
filenameslist.sort()

#print filenameslist

# https://en.wikipedia.org/wiki/Binomial_coefficient#Binomial_coefficient_in_programming_languages
# it's no use, too big.
def binomial(n, k):
    if k < 0 or k > n:
        return 0
    if k > n - k: # take advantage of symmetry
        k = n - k
    c = 1
    for i in range(k):
        c = c * (n - (k - (i+1)))
        c = c // (i+1)
    return c

# use this instead.
# http://www.biostars.org/post/show/9659/the-problem-with-converting-long-int-to-float-in-python/
from math import log, exp
from gamma import log_gamma
def logchoose(ni, ki):
	try:
		lgn1 = log_gamma(ni+1)
		lgk1 = log_gamma(ki+1)
		lgnk1 = log_gamma(ni-ki+1)
	except ValueError:
		#print ni,ki
		raise ValueError
	return lgn1 - (lgnk1 + lgk1)

def make_plots(csv_file):
	csvreader = csv.reader(open(csv_file,'r'))
	rows = (csvreader)
	cols = zip(*csvreader)
	cols = dict((c[0],c[1:]) for c in cols)
	print cols.keys()
	def float_if_possible(x):
		try:
			return float(x)
		except ValueError:
			return x
	for k,v in cols.items():
		cols[k] = [float_if_possible(x) for x in v]

	def mean(list):
		return sum(list) / len(list)
	rows = zip(cols['p'],cols['neighbors'], cols['population size'],
			cols['update rule'],cols['proportion adopting'],
			cols['last action'])
	rows.sort(key=lambda row: row[0]) # sort by p
	rows.sort(key=lambda row: row[2]) # then by lattice size
	rows.sort(key=lambda row: row[3]) # then by update rule
	rows.sort(key=lambda row: row[1]) # then by neighborhood size
	mean_plots = []
	prob_plots = []
	last_plots = []
	for nb, rows_nb in groupby(rows, lambda row: row[1]):
		for r, rows_r in groupby(rows_nb, lambda row: row[3]):
			for n, rows_n in groupby(rows_r, lambda row: row[2]):
				p_pairs = [(p,[row for row in p_rows])
						for p, p_rows in groupby(rows_n, lambda row: row[0])]
				mean_pairs = ((p,mean([row[4] for row in p_rows]))
					for p, p_rows in p_pairs)
				pr = zip(*(sorted(mean_pairs, key = lambda pair: pair[0])))
				mean_plots.append((pr, r, n, nb))
				prob_success_pairs = ((p,
							(1.0*len([pa[4] for pa in p_rows if pa[4] > 0.5])
								/len(p_rows)))
						for p, p_rows in p_pairs)
				pr = zip(*(sorted(prob_success_pairs, key = lambda pair: pair[0])))
				prob_plots.append((pr, r, n, nb))
				last_pairs = ((p,mean([pa[5] for pa in p_rows]))
						for p, p_rows in p_pairs)
				pr = zip(*(sorted(last_pairs, key = lambda pair: pair[0])))
				last_plots.append((pr, r, n, nb))

	summaries_filename = csv_file.rstrip("csv") + "mean.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	for pr, r, n, nb in mean_plots:
		plt.plot( pr[0], pr[1], label="%d neighbors, %s"%(nb,r) )
	plt.plot( pr[0], pr[0], 'k,' )
	plt.suptitle("Mean density of adoption")
	plt.legend(loc="lower right", prop={"size":8})
	fig.savefig(summaries_filename);

	summaries_filename = csv_file.rstrip("csv") + "probability.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	for pr, r, n, nb in prob_plots:
		plt.plot( pr[0], pr[1], label="%d neighbors, %s"%(nb,r) )
	n = int(cols['population size'][0])
	ks = range((n+1)/2,n+1)
	pm = [(sum(exp(logchoose(n,k)+k*log(p)+(n-k)*log(1-p)) for k in ks) if p < 1
		else 0) for p in pr[0]]
	plt.plot( pr[0], pm, 'k:')
	plt.xlim(xmax=0.6)
	plt.suptitle("Probability that majority adopts")
	plt.legend(loc="lower right", prop={"size":8})
	fig.savefig(summaries_filename);

	summaries_filename = csv_file.rstrip("csv") + "last.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	for pr, r, n, nb in last_plots:
		plt.plot( pr[0], pr[1], label="%d neighbors, %s"%(nb,r) )
	plt.plot( pr[0], pr[0], 'k,' )
	plt.suptitle("Probability that last player adopts")
	plt.legend(loc="lower right", prop={"size":8})
	fig.savefig(summaries_filename);

for csv_file in filenameslist:
	make_plots(csv_file)

