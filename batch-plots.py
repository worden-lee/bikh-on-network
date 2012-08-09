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

	def mean(list):
		return sum(list) / len(list)
	coloration = [ 'r-', 'b-', 'g-', 'm-', 'c-', 'y-' ]
	rows = zip(cols['p'],cols['neighbors'], cols['population size'],
			cols['update rule'],cols['proportion adopting'])
	rows.sort(key=lambda row: row[0]) # sort by p
	rows.sort(key=lambda row: row[2]) # then by lattice size
	rows.sort(key=lambda row: row[1]) # then by neighborhood size
	rows.sort(key=lambda row: row[3]) # then by update rule
	mean_plots = []
	prob_plots = []
	c_index = 0
	for r, rows_r in groupby(rows, lambda row: row[3]):
		for nb, rows_nb in groupby(rows_r, lambda row: row[1]):
			for n, rows_n in groupby(rows_nb, lambda row: row[2]):
				p_pairs = [(p,[row[4] for row in rows_p])
						for p, rows_p in groupby(rows_n, lambda row: row[0])]
				mean_pairs = ((p,mean(p_adoption_list))
					for p, p_adoption_list in p_pairs)
				pr = zip(*(sorted(mean_pairs, key = lambda pair: pair[0])))
				mean_plots.append((pr, r, n, nb, c_index))
				prob_success_pairs = ((p,
					(len([pa for pa in p_adoption_list if pa > 0.5])/len(p_adoption_list)))
						for p, p_adoption_list in p_pairs)
				pr = zip(*(sorted(prob_success_pairs, key = lambda pair: pair[0])))
				prob_plots.append((pr, r, n, nb, c_index))
				c_index = c_index + 1
	summaries_filename = csv_file.rstrip("csv") + "mean.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	for pr, r, n, nb, c_index in mean_plots:
		plt.plot( pr[0], pr[1], coloration[c_index], 
				label="%d neighbors, %s"%(nb,r) )
	plt.plot( pr[0], pr[0], 'k,' )
	plt.suptitle("Mean density of adoption")
	plt.legend(loc="lower right", prop={"size":8})
	fig.savefig(summaries_filename);

	summaries_filename = csv_file.rstrip("csv") + "probability.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	for pr, r, n, nb, c_index in prob_plots:
		plt.plot( pr[0], pr[1], coloration[c_index], 
				label="%d neighbors, %s"%(nb,r) )
	plt.suptitle("Probability that majority adopts")
	plt.legend(loc="lower right", prop={"size":8})
	fig.savefig(summaries_filename);

for csv_file in filenameslist:
	make_plot(csv_file)

