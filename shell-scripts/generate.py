#!/usr/bin/python3

import sys
import subprocess
import re
import fileinput
import os
import contextlib
import mmap
import shutil
import glob

times = 5
rp_names = ['BEHDS', 'GOAL', 'GPSR']
cbr_numbers = [100]
epsilons = [0.8, 1.2, 1.5, 2.5, 3, 3.5]
num = [4,6,8]
knum = [2,6]

d = [
{
    'e': 0.5, 'n': 14, 'k': 1
},
{
    'e': 0.8, 'n': 14, 'k': 1
},
{
    'e': 1.2, 'n': 14, 'k': 1  
},
{
    'e': 1.5, 'n': 14, 'k': 1
},
{
    'e': 2, 'n': 14, 'k': 1
},

]

# for i in range(times):
#     cdir = './times' + str(i) + '/'
#     for rp in rp_names:
#         with fileinput.input('simulate.tcl', inplace=True) as file:
#             for line in file:
#                 line = re.sub(
#                     r'set opt\(rp\).*', 'set opt(rp) {0}'.format(rp), line.rstrip())
#                 print(line)
#         for ps in packet_sizes:
#             with fileinput.input('simulate.tcl', inplace=True) as file:
#                 for line in file:
#                     line = re.sub(
#                         r'Agent/CBR set packetSize_.*', 'Agent/CBR set packetSize_ {0}'.format(ps), line.rstrip())
#                     print(line)
#             newpath = '{0}-{1}'.format(rp, ps)
#             if not os.path.exists(cdir + newpath):
#                 os.makedirs(cdir + newpath)

#             for file in glob.glob(r'./*.tcl'):
#                 shutil.copy(file, cdir + newpath)

# for i in range(times):
#     cdir = './times' + str(i) + '/'
#     for e in epsilons:
#         with fileinput.input('simulate.tcl', inplace=True) as file:
#             for line in file:
#                 line = re.sub(
#                     r'Agent/CORBAL set epsilon_.*', 'Agent/CORBAL set epsilon_ {0}'.format(e), line.rstrip())
#                 print(line)
#         for n in num:
#             with fileinput.input('simulate.tcl', inplace=True) as file:
#                 for line in file:
#                     line = re.sub(
#                         r'Agent/CORBAL set n_.*', 'Agent/CORBAL set n_ {0}'.format(n), line.rstrip())
#                     print(line)
#             for k in knum:
#                 with fileinput.input('simulate.tcl', inplace=True) as file:
#                     for line in file:
#                         line = re.sub(
#                             r'Agent/CORBAL set k_n_.*', 'Agent/CORBAL set k_n_ {0}'.format(k), line.rstrip())
#                         print(line)

#                 newpath = 'CORBAL-{0}-{1}-{2}'.format(n, k, e)
#                 if not os.path.exists(cdir + newpath):
#                     os.makedirs(cdir + newpath)

#                 for file in glob.glob(r'./*.tcl'):
#                     shutil.copy(file, cdir + newpath)

for i in range(times):
    cdir = './times' + str(i) + '/'
    for t in d:
        with fileinput.input('simulate.tcl', inplace=True) as file:
            for line in file:
                line = re.sub(
                    r'Agent/CORBAL set epsilon_.*', 'Agent/CORBAL set epsilon_ {0}'.format(t['e']), line.rstrip())
                line = re.sub(
                    r'Agent/CORBAL set n_.*', 'Agent/CORBAL set n_ {0}'.format(t['n']), line.rstrip())
                line = re.sub(
                    r'Agent/CORBAL set k_n_.*', 'Agent/CORBAL set k_n_ {0}'.format(t['k']), line.rstrip())
                print(line)

        newpath = 'CORBAL-{0}-{1}-{2}'.format(t['n'], t['k'], t['e'])
        if not os.path.exists(cdir + newpath):
            os.makedirs(cdir + newpath)

        for file in glob.glob(r'./*.tcl'):
            shutil.copy(file, cdir + newpath)