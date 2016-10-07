import os
import subprocess
import shutil

print "[+] Enter root directory: "
rootdir = raw_input()
for root, subFolders, files in os.walk(rootdir):
    if 'simulate.tcl' in files:
        shutil.copy("autorunbcp.py", root)
        os.chdir(root)
        print(root)
        args = "python3 autorunbcp.py".split()
        # args = "ns simulate.tcl".split()
        popen = subprocess.Popen(args, stdout=subprocess.PIPE)
        popen.wait()
        output = popen.stdout.read()
        print(output)