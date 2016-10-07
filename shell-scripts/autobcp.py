#!/usr/bin/python3

import sys
import subprocess
import re
import fileinput
import os

node_count = int(sys.argv[1])
num = 0
while True:
	args = "./ns simulate.tcl".split()
	popen = subprocess.Popen(args, stdout=subprocess.PIPE)
	popen.wait()
	output = popen.stdout.read()
	print(output)
	output = str(output)
	regex = re.compile('NewPointX:([0-9\.]+)NewPointY:([0-9\.]+)')
	m = regex.findall(output)
	if len(m) == 0:
		sys.exit(1)

	for mm in m:
		(x, y) = mm
		print('Continue ... ')

		node_count += 1
		num += 1
		string = "$mnode_({0}) set X_ {1} ;	$mnode_({0}) set Y_ {2} ;	$mnode_({0}) set Z_ 0\n".format(node_count, x, y)
		with open("topo_data.tcl", "a") as myfile: myfile.write(string)

	with fileinput.input('simulate.tcl', inplace=True) as file:
		for line in file:
			line = re.sub(r'set opt\(nn\).*', 'set opt(nn)	{0}'.format(node_count+1), line.rstrip())
			print(line)
	print("copying...")
	for filename in os.listdir("."):
		if filename == "Runtime.tr":
			os.rename(filename, "Runtime_{0}.tr".format(num))
		elif filename == "Energy.tr":
			os.rename(filename, "Energy_{0}.tr".format(num))
