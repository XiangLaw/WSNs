# for j in `seq 2 10`;
# do
# 	for i in `seq 1 20`;
# 	do
# 	    cd /Volumes/#Dandoh/Coverage/$j/s$i/
# 		echo `pwd`
# 	    /Users/huyvq/workspace/thesis/ns-allinone-2.35/bin/ns simulate.tcl

# 	    # find . -name "*.tr" -delete
# 	    # cp /Users/huyvq/workspace/tmp/autorunbcp.py /Users/huyvq/workspace/thesis/ns-allinone-2.35/ns-2.35/scripts/exp/
# 	    # cp /Users/huyvq/workspace/tmp/hpa/s$i/*.tcl /Users/huyvq/workspace/thesis/ns-allinone-2.35/ns-2.35/scripts/exp/
# 	    # python3 autorunbcp.py
	    
# 	    # /Users/huyvq/workspace/thesis/ns-allinone-2.35/bin/ns simulate.tcl

# 	    # mkdir -p /Users/huyvq/workspace/tmp/coverage-latency/s$i/ 
# 	    # cp /Users/huyvq/workspace/thesis/ns-allinone-2.35/ns-2.35/scripts/exp/EnergyTracer.tr /Users/huyvq/workspace/tmp/coverage-latency/s$i/ 
# 	done
# done

# cd /Volumes/#Dandoh/Coverage
# /Users/huyvq/workspace/thesis/ns-allinone-2.35/bin/ns simulate.tcl

# cd /Volumes/#Dandoh/aloba/gpsr-10000
# /Users/huyvq/workspace/thesis/ns-allinone-2.35/bin/ns simulate.tcl

# cd /Volumes/#Dandoh/aloba/aloba-test6/CORBAL-8-3-2.0/
# /Users/huyvq/workspace/thesis/ns-allinone-2.35/bin/ns simulate.tcl


for f in $1/*;
  do
    [ -d $f ] && cd "$f" && echo $f
    for ff in $f/*;
	do
	    [ -d $ff ] && cd "$ff" && echo $ff
		/Users/huyvq/workspace/thesis/ns-allinone-2.35/bin/ns simulate.tcl
	done;
	# /Users/huyvq/workspace/thesis/ns-allinone-2.35/bin/ns simulate.tcl
done;

# for i in `seq 1 20`;
# do
#     find . -name "*.tr" -delete
#     cp /Users/huyvq/workspace/tmp/autorunbcp.py /Users/huyvq/workspace/thesis/ns-allinone-2.35/ns-2.35/scripts/exp/
#     cp /Users/huyvq/workspace/tmp/hpa/s$i/*.tcl /Users/huyvq/workspace/thesis/ns-allinone-2.35/ns-2.35/scripts/exp/
#     python3 autorunbcp.py
    
#     mkdir -p /Users/huyvq/workspace/tmp/dump-hach-latency/s$i/ 
#     cp /Users/huyvq/workspace/thesis/ns-allinone-2.35/ns-2.35/scripts/exp/*.tr /Users/huyvq/workspace/tmp/dump-hach-latency/s$i/ 
# done

# cd /Volumes/#Dandoh/dump-hach-latency
# for i in `seq 1 20`;
# do
#     cd s$i;
#     wc PatchingHole.tr
#     cd ..;
# done
