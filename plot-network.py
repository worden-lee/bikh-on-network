import sys
import csv
import re
import pygraphviz

# input file is csv of vertex,neigbors
# where vertex is a number and neighbors is a space-separated list of numbers
input_filename = sys.argv[1]
# we will convert it to a .dot file
dot_filename = input_filename.rstrip( '.csv' ) + '.dot';
# and plot it into a png file
png_filename = input_filename.rstrip( '.csv' ) + '.png';

input_rows = csv.DictReader( open( sys.argv[1], 'r' ) )

graph_dict = dict( ( ( row['vertex'],
	  dict( ( (n, None) for n in re.split(' ', row['neighbors']) 
	    if n != '' ) )
	) for row in input_rows ) )
graph = pygraphviz.AGraph( graph_dict, strict=False, directed=False )

graph.graph_attr['size'] = '5,5';
graph.graph_attr['nodesep'] = '1.5';
graph.graph_attr['K'] = '10';
graph.node_attr['shape'] = 'circle';
graph.node_attr['color'] = 'red';
graph.node_attr['height'] = '0.01';
graph.node_attr['style'] = 'filled';
graph.node_attr['label'] = ' ';

graph.write( dot_filename )

graph.draw( png_filename, prog='twopi' )
