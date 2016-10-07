#!/usr/bin/python3

'''
03.19.2016 - 15:04
'''
import sys
import subprocess
import re
import fileinput
import os
import contextlib
import mmap

pattern = re.compile(b'set opt\(nn\).([0-9\.]+)')

with open('simulate.tcl', 'r') as f:
    with contextlib.closing(mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)) as m:
        for match in pattern.findall(m):
            node_count = int(match)

print('num node = %d' % node_count)
num = 0
while True:
    args = "ns simulate.tcl".split()
    popen = subprocess.Popen(args, stdout=subprocess.PIPE)
    popen.wait()
    output = popen.stdout.read()
    print(output)
    output = str(output)
    regex = re.compile('NewPointX:([0-9\.]+)NewPointY:([0-9\.]+)')
    m = regex.findall(output)
    if len(m) == 0:
        sys.exit(1)

    if num == 0:
        for filename in os.listdir("."):
            if filename == "CoverageBoundHole.tr":
                os.rename(filename, "CoverageBoundHole_org.tr")
            elif filename == "Trace.tr":
                os.rename(filename, "Trace_org.tr")
            elif filename == "Neighbors.tr":
                os.rename(filename, "Neighbors_org.tr")

    for mm in m:
        (x, y) = mm
        print('Continue ... ')

        node_count += 1
        num += 1
        string = "$mnode_({0}) set X_ {1} ; $mnode_({0}) set Y_ {2} ;   $mnode_({0}) set Z_ 0\n".format(
            node_count-1, x, y)
        with open("topo_data.tcl", "a") as myfile:
            myfile.write(string)

    with fileinput.input('simulate.tcl', inplace=True) as file:
        for line in file:
            line = re.sub(
                r'set opt\(nn\).*', 'set opt(nn)  {0}'.format(node_count), line.rstrip())
            print(line)
    print("copying...")
    for filename in os.listdir("."):
        if filename == "EnergyTracer.tr":
            os.rename(filename, "EnergyTracer{0}.tr".format(num))
