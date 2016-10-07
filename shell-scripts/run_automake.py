import os
import subprocess
import re
import fileinput

ns_path = "/Users/huyvq/workspace/thesis/ns-allinone-2.35/ns-2.35/"
corbal_path = "wsn/corbal/"
cb_packet = ns_path + corbal_path + "corbal_packet.h"

print("[+] Enter root directory: ")
rootdir = input()
for root, subFolders, files in os.walk(rootdir):
    if "number" in root.split(os.path.sep)[-1]:
        print(root)
        num = root.split(os.path.sep)[-1].split("number")[1]
        with fileinput.input(cb_packet, inplace=True) as file:
            for line in file:
                line = re.sub(
                    r'    Point routing_table.*', '    Point routing_table[{0}];'.format(num), line.rstrip())
                line = re.sub(
                    r'        return sizeof\(u_int8_t\) \* 3 \+ 3 \* sizeof\(Point\) \+.*', '        return sizeof(u_int8_t) * 3 + 3 * sizeof(Point) + {0} * sizeof(Point);'.format(num), line.rstrip())
                print(line)
        os.chdir(ns_path)
        print("Re-compiling....")
        args = "make clean".split()
        popen = subprocess.Popen(args, stdout=subprocess.PIPE)
        popen.wait()
        output = popen.stdout.read()
        print(output)
        args = "make".split()
        popen = subprocess.Popen(args, stdout=subprocess.PIPE)
        popen.wait()
        print("Re-compiling.... Done!")
    if 'simulate.tcl' in files:
        print("Running simulation....")
        os.chdir(root)
        print(root)
        # args = "ns simulate.tcl".split()
        # popen = subprocess.Popen(args, stdout=subprocess.PIPE)
        # popen.wait()
        # output = popen.stdout.read()
        # print(output)
        print("Running simulation.... Done!")