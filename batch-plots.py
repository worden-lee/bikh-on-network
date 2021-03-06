import matplotlib
matplotlib.use('agg')
from pylab import *
import scipy

from scipy import special
import csv

from itertools import groupby

# import numpy as np
import os, fnmatch
import sys

if (len(sys.argv) >= 2):
	filenameslist = [sys.argv[1]]
else:
	filenameslist = fnmatch.filter(os.listdir("."),"*.microstate.csv")
filenameslist.sort()

#print filenameslist

# regular binomial fails on big numbers, use this instead.
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
	rows = [csvreader]
	cols = zip(*csvreader)
	cols = dict((c[0],c[1:]) for c in cols)
	def float_if_possible(x):
		try:
			return float(x)
		except ValueError:
			return x
	for k,v in cols.items():
		cols[k] = [float_if_possible(x) for x in v]

	def mean(list):
		return sum(list) / len(list)
	colnames = ['p', 'neighbors', 'population size', 'update rule',
		'inference closure level', 'proportion adopting',
		'last action']

	#rows = zip(cols['p'],cols['neighbors'], cols['population size'],
	#		cols['update rule'], cols['inference closure level'],
	#		cols['proportion adopting'],
	#		cols['last action'])
	rows = zip( *[cols[n] for n in colnames] )
	rows.sort(key=lambda row: row[0]) # sort by p
	rows.sort(key=lambda row: row[2]) # then by lattice size
	rows.sort(key=lambda row: row[3]) # then by update rule
	rows.sort(key=lambda row: row[4]) # then by closure level (applicable to 'bayesian-closure' rule only)
	rows.sort(key=lambda row: row[1]) # then by neighborhood size
	def extended_rule_name(rule, level):
		if ( rule == 'bayesian-closure' ):
			return '%s-%s' % (rule, int(level))
		if ( rule == 'bayesian' ):
			return 'Full Bayesian Inference';
		if ( rule == 'bayesian-same-neighborhood' ):
			return 'Bayesian Inference';
		if ( rule == 'counting' ):
			return 'Counting';
		return rule;
	rows = [ (p, n, s, extended_rule_name(r,l), pa, la) for (p, n, s, r, l, pa, la) in rows ]
	mean_plots = []
	prob_plots = []
	last_plots = []
	count_csvfilename = csv_file.rstrip("csv") + "frequencies.csv"
	print count_csvfilename
	count_csvfile = open( count_csvfilename, 'wb' )
	count_csvwriter = csv.writer( count_csvfile)
	count_csvwriter.writerow( [ 'p', 'neighborhood size', 'population size', 'rule', 'frequency' ] )
	for nb, rows_nb in groupby(rows, lambda row: row[1]):
		for r, rows_r in groupby(rows_nb, lambda row: row[3]):
			for n, rows_n in groupby(rows_r, lambda row: row[2]):
				p_pairs = [(p,[row for row in p_rows])
						for p, p_rows in groupby(rows_n, lambda row: row[0])]
				for p, p_rows in p_pairs:
					count_csvwriter.writerow( [ p, nb, n, r, len(p_rows) ] )
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
	plt.suptitle("Mean density of adoption")
	plt.xlabel("Probability of positive signal")
	for pr, r, n, nb in mean_plots:
		plt.plot( pr[0], pr[1], label="%d neighbors, %s"%(nb,r) )
	plt.plot( pr[0], pr[0], 'k,', label="independent" )
	# caution: this is just using the last value of nb
	cascade_prob = [ (p*(p+1)*(1- (p*(1-p))**(nb/2)))/(2*(1-p*(1-p))) + (p*(1-p))**(nb/2)/2 for p in pr[0] ]
	plt.plot( pr[0], cascade_prob, 'k--', label="global cascade" )
	plt.xlim(xmax=1)
	plt.legend(loc="lower right", prop={"size":8})
	fig.savefig(summaries_filename);

	summaries_filename = csv_file.rstrip("csv") + "probability.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	plt.suptitle("Probability that majority adopts")
	plt.xlabel("Probability of positive signal")
	for pr, r, n, nb in prob_plots:
		plt.plot( pr[0], pr[1], label="%d neighbors, %s"%(nb,r) )
	n = int(cols['population size'][0])
	ks = range((n+1)/2,n+1)
	pm = [(sum(exp(logchoose(n,k)+k*log(p)+(n-k)*log(1-p)) for k in ks) if p < 1
		else 0) for p in pr[0]]
	plt.plot( pr[0], pm, 'k:', label="independent" )
	plt.plot( pr[0], cascade_prob, 'k--', label="global cascade" )
	plt.xlim(xmax=0.6)
	plt.legend(loc="lower right", prop={"size":8})
	fig.savefig(summaries_filename);
  
	summaries_filename = csv_file.rstrip("csv") + "last.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	plt.suptitle("Probability that last player adopts")
	plt.xlabel("Probability of positive signal")
	for pr, r, n, nb in last_plots:
		plt.plot( pr[0], pr[1], label="%d neighbors, %s"%(nb,r) )
	plt.plot( pr[0], pr[0], 'k,', label="independent")
	plt.plot( pr[0], cascade_prob, 'k--', label="global cascade" )
	plt.xlim(xmax=1)
	plt.legend(loc="lower right", prop={"size":8})
	fig.savefig(summaries_filename);
  
	rows.sort(key=lambda row: row[1]) # now re-sort by neighborhood size
	size_data = []
	for r, rows_r in groupby((row for row in rows if float(row[0]) == 0.55), lambda row: row[3]):
		for n, rows_n in groupby(rows_r, lambda row: row[2]):
			for nb, rows_nb in groupby(rows_n, lambda row: row[1]):
				ld = [[row[5],row[4]] for row in rows_nb]
				ld = zip(*ld)
				lastp = mean(ld[0])
				dens = mean(ld[1])
				prob = (1.0 * len( [q for q in ld[1] if q > 0.5] ) / len(ld[1]))
				size_data.append([nb, r, n, lastp, dens, prob])
	
	summaries_filename = csv_file.rstrip("csv") + "size-last.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	size_data.sort(key=lambda row: row[2]) # sort by n
	size_data.sort(key=lambda row: row[1]) # sort by r
	for r, r_rows in groupby(size_data, lambda row: row[1]):
		for n, n_rows in groupby(r_rows, lambda row: row[2]):
			nz = [[nb, mean([row[3] for row in nb_rows])]
				for nb, nb_rows in groupby(n_rows, lambda row: row[0])]
			n_z = zip(*nz)
			plot( n_z[0], n_z[1], label=r )
	#plt.plot( pr[0], pr[0], 'k,', label="independent" )
	plt.ylim(ymax=1)
	plt.suptitle("%g players, p=0.55"%n)
	plt.ylabel("probability that last player adopts")
	plt.xlabel("neighbourhood size")
	plt.legend(loc='upper right', prop={"size":8})
	plt.subplots_adjust(bottom=.10,left=.14)
	fig.savefig(summaries_filename);

	summaries_filename = csv_file.rstrip("csv") + "size-mean.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	spl = plt.subplot(111)
	size_data.sort(key=lambda row: row[2]) # sort by n
	size_data.sort(key=lambda row: row[1]) # sort by r
	ymax = 0
	for r, r_rows in groupby(size_data, lambda row: row[1]):
		for n, n_rows in groupby(r_rows, lambda row: row[2]):
			nz = [[nb, mean([row[4] for row in nb_rows])]
				for nb, nb_rows in groupby(n_rows, lambda pair: pair[0])]
			n_z = zip(*nz)
			plot( n_z[0], n_z[1], label=r )
			ymax = max(ymax, *n_z[1])
	if ymax > 0.7 :
		plt.ylim(ymax=1.01)
	#plt.suptitle("%g players, p=0.55"%n)
	#plt.ylabel("density of adoption")
        plt.suptitle( "Density of adoption" )
	plt.xlabel("Neighborhood size")
	#plt.legend(loc="upper right", prop={"size":8})
	plt.subplots_adjust(bottom=.10,left=.18)
        # ad hoc: too many ticks for figure
        spl.set_xticks( spl.get_xticks()[::2] )
	fig.savefig(summaries_filename);

	summaries_filename = csv_file.rstrip("csv") + "size-probability.png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	spl = plt.subplot(111)
	size_data.sort(key=lambda row: row[2]) # sort by n
	size_data.sort(key=lambda row: row[1]) # sort by r
	ymin = 1
	for r, r_rows in groupby(size_data, lambda row: row[1]):
		for n, n_rows in groupby(r_rows, lambda row: row[2]):
			nz = [[nb, mean([row[5] for row in nb_rows])]
				for nb, nb_rows in groupby(n_rows, lambda pair: pair[0])]
			n_z = zip(*nz)
			plot( n_z[0], n_z[1], label=r )
			ymin = min(ymin, n_z[1])
	#plt.plot( pr[0], pr[0], 'k,' )
	plt.ylim(ymax=1.01)
	#plt.suptitle("%g players, p=0.55"%n)
	#plt.ylabel("probability of majority adoption")
        plt.suptitle( "Probability of majority adoption" )
	plt.xlabel("Neighborhood size")
	if ymin < 0.95 :
		pass #plt.legend(loc="upper right", prop={"size":8})
	else :
		plt.ylim(ymin=0.5)
		#plt.legend(loc="lower right", prop={"size":8})
	plt.subplots_adjust(bottom=.10,left=.18)
        # ad hoc: too many ticks for figure
        spl.set_xticks( spl.get_xticks()[::2] )
	fig.savefig(summaries_filename);

for csv_file in filenameslist:
	make_plots(csv_file)

