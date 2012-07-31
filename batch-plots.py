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
	csvreader = csv.DictReader(open(csv_file,'r'))
	rows = (csvreader)
	summaries_filename = csv_file.rstrip("csv") + "png"
	print summaries_filename
	fig = plt.figure(figsize=(4,4))
	plt.subplot(111)
	plt.plot( (float(row['p']) for row in rows), 
			(float(row['proportion adopting']) for row in rows), 'r,') 
	fig.savefig(summaries_filename);
	++next_frame_t

for csv_file in filenameslist:
	make_plot(csv_file)

