import matplotlib
matplotlib.use('agg')
from pylab import *
import scipy

from scipy import special
import csv

from itertools import groupby

# import numpy as np
import numpy.arange

### Packages which are necessary to create a movie
 
#import subprocess                 # For issuing commands to the OS.
import os, fnmatch
import sys
from matplotlib.text import Text
import gc

#import memory_profile

if (len(sys.argv) >= 2):
	filenameslist = [sys.argv[1]]
else:
	filenameslist = fnmatch.filter(os.listdir("."),"*.microstate.csv")
filenameslist.sort()

#print filenameslist

fig = 0
 
def save_frame(data, filename_base, frame_number, nb, rule):
	frame_filename = filename_base + "%06g.frame.png"%float(frame_number)
	if ( not os.path.exists(frame_filename) ):
		print frame_filename
		global fig
		if fig != 0:
			fig.clear();
		fig = plt.figure(figsize=(4,4))
		plt.suptitle("%s neighbors, %s"%(nb,rule))
		plt.subplot(111)
		plt.imshow(data, interpolation="nearest")
		fig.savefig(frame_filename)
		Text.cached = {}
		gc.collect()

def make_microstate_plots(csv_file):
	settings_file = csv_file.rstrip("microstate.csv")+"settings.csv"
	settings = dict(csv.reader(open(settings_file)))
	try:
		nb = settings['n_neighbors']
	except KeyError:
		nb = 'all'
	rule = settings['update_rule']
	#print settings
	csvreader = csv.DictReader(open(csv_file,'r'))
	#csvheader = csvreader.next()
	#csvindices = dict(((k,i) for k,i in zip(csvheader, range(len(csvheader)))))
	def float_if_possible(x):
		try:
			return float(x)
		except ValueError:
			return x
	def color(decided, adopted, cascaded, flipped):
		if not decided:
			return [1,1,1]
		elif adopted:
			if cascaded:
				return [1,0.7,0.7]
			elif flipped:
				return [1,0.3,0.3]
			else:
				return [1,0,0]
		else:
			if cascaded:
				return [0.7,0.7,1]
			elif flipped:
				return [0.3,0.3,1]
			else:
				return [0,0,1]
	X = ones( (int(settings['lattice_dim_0']),int(settings['lattice_dim_1']),3), float )
	next_frame_t = 0
	n_adopted = 0
	n_counted = 0
	cumulative_density = []
	for row in csvreader:
		#if float(row['t']) > 300:
		#	break
		n_adopted += row['adopted']
		++n_counted;
		cumulative_density = cumulative_density + [ n_adopted / float(n_counted) ]
		if float(row['t']) >= next_frame_t:
			save_frame(X, csv_file.rstrip("csv"), next_frame_t, nb, rule)
			next_frame_t = next_frame_t + 1
		X[row['x'],row['y'],:] = color(
				int(row['decided']), 
				int(row['adopted']), 
				int(row['cascaded']),
				int(row['flipped']))
		#print row, X[row['x'],row['y'],:]
	save_frame(X, csv_file.rstrip("csv"), next_frame_t, nb, rule)
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	plt.suptitle("Cumulative probability of adoption")
	plt.xlabel("Time")
	plt.plot( numpy.arange( 0, 1.0 / length( cumulative_adoption ), 1 ), cumulative_adoption );
	fig.savefig(csv_file.rstrip("microstate.csv") + 'cumulative-density.png');

for csv_file in filenameslist:
	make_microstate_plots(csv_file)


