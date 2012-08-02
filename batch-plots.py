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
	print cols.keys()
	def float_if_possible(x):
		try:
			return float(x)
		except ValueError:
			return x
	for k,v in cols.items():
		cols[k] = [float_if_possible(x) for x in v]

	summaries_filename = csv_file.rstrip("csv") + "png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)

	def mean(list):
		return sum(list) / len(list)
	coloration = [ 'r-', 'b-', 'g-', 'p-' ]
	c_index = 0
	rows = zip(cols['p'],cols['neighbors'], cols['population size'],cols['proportion adopting'])
	rows.sort(key=lambda row: row[0]) # sort by p
	rows.sort(key=lambda row: row[2]) # then by lattice size
	rows.sort(key=lambda row: row[1]) # then by neighborhood size
	for nb, rows_nb in groupby(rows, lambda row: row[1]):
		for n, rows_n in groupby(rows_nb, lambda row: row[2]):
			means = {}
			pairs = ((p,mean([row[3] for row in rows_p])) 
				for p, rows_p in groupby(rows_n, lambda row: row[0]))
			pr = zip(*(sorted(pairs, key = lambda pair: pair[0])))
			plt.plot( pr[0], pr[1], coloration[c_index], 
					label="size %d, %d neighbors"%(n,nb) )
			c_index = c_index + 1
	plt.plot( pr[0], pr[0], 'k,' )
	plt.legend()
	fig.savefig(summaries_filename);

for csv_file in filenameslist:
	make_plot(csv_file)

