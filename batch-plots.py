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

def make_plot(csv_file):
	csvreader = csv.reader(open(csv_file,'r'))
	rows = (csvreader)
	cols = zip(*csvreader)
	cols = dict((c[0],c[1:]) for c in cols)
	def float_if_possible(x):
		try:
			return float(x)
		except ValueError:
			return x
	for k,v in cols.items():
		cols[k] = [float_if_possible(x) for x in v]
	means = {}
	def mean(list):
		return sum(list) / len(list)
	pairs = ((p,mean([pair[1] for pair in rows])) 
		for p, rows in groupby(zip(cols['p'], cols['proportion adopting']),
				lambda pair: pair[0]))
	pr = zip(*(sorted(pairs, key = lambda pair: pair[0])))
	print pr
	summaries_filename = csv_file.rstrip("csv") + "png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	plt.plot( pr[0], pr[0], 'k,' )
	plt.plot( pr[0], pr[1], 'r-' )
	fig.savefig(summaries_filename);

for csv_file in filenameslist:
	make_plot(csv_file)

