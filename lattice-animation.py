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

#import memory_profile

if (len(sys.argv) >= 2):
	filenameslist = [sys.argv[1]]
else:
	filenameslist = fnmatch.filter(os.listdir("."),"*.microstate.csv")
filenameslist.sort()

#print filenameslist

number_of_txtfiles = len(filenameslist)
 
@profile
def make_frames(csv_file):
	settings_file = csv_file.rstrip("microstate.csv")+"settings.csv"
	settings = dict(csv.reader(open(settings_file)))
	#print settings
	csvreader = csv.DictReader(open(txt_file,'r'))
	#csvheader = csvreader.next()
	#csvindices = dict(((k,i) for k,i in zip(csvheader, range(len(csvheader)))))
	def float_if_possible(x):
		try:
			return float(x)
		except ValueError:
			return x
	#for c in csvcolumns:
	#	csvdata[c[0]] = [float_if_possible(x) for x in c[1:]]
	def color(decided, adopted, cascaded, flipped):
		if not decided:
			return [1,1,1]
		elif adopted:
			if cascaded:
				return [1,0.7,0.7]
			elif flipped:
				return [1,0.4,0.4]
			else:
				return [1,0,0]
		else:
			if cascaded:
				return [0.7,0.7,0.7]
			elif flipped:
				return [0.4,0.4,0.4]
			else:
				return [0,0,0]
	X = ones( (int(settings['lattice_dim_0']),int(settings['lattice_dim_1']),3), float )
	next_frame_t = 0
	for row in csvreader:
		#if float(row['t']) > 300:
		#	break
		X[row['x'],row['y'],:] = color(
				int(row['decided']), 
				int(row['adopted']), 
				int(row['cascaded']),
				int(row['flipped']))
		print row, X[row['x'],row['y'],:]
		if float(row['t']) >= next_frame_t:
			frame_filename = txt_file.rstrip("csv") + "%06g.frame.png"%float(row['t'])
			if ( not os.path.exists(frame_filename) ):
				print frame_filename
				fig = plt.figure(figsize=(4,4))
				plt.subplot(111)
				plt.imshow(X, interpolation="nearest")
				fig.savefig(frame_filename);
			++next_frame_t

for txt_file in filenameslist:
	make_frames(txt_file)

