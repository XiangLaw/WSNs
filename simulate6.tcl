# Script for WisSim simulator. Last edit 3/14/2015 9:05:53 AM

set opt(x)	1000	;# X dimension of the topography
set opt(y)	1000	;# Y dimension of the topography
set opt(stop)	200	;# simulation time
set opt(nn)	1500	;# number of nodes
set opt(tr)	Trace.tr	;# trace file
set opt(nam)	nam.out.tr

set opt(ifqlen)	50	;# max packet in ifq
set opt(chan)	Channel/WirelessChannel
set opt(prop)	Propagation/TwoRayGround
set opt(netif)	Phy/WirelessPhy
set opt(mac)	Mac/802_11
set opt(ifq)	Queue/DropTail/PriQueue
set opt(ll)	LL
set opt(ant)	Antenna/OmniAntenna
set opt(rp)	ELLIPSE
set opt(trans)	UDP
set opt(apps)	CBR

set opt(energymodel)	EnergyModel
set opt(initialenergy)  1000		;# Initial energy in Joules
set opt(checkpoint)	995
set opt(idlePower) 	0.0096
set opt(rxPower) 	0.021
set opt(txPower) 	0.0255
set opt(sleepPower) 	0.000648
set opt(transitionPower) 0.024
set opt(transitionTime)  0.0129

# ======================================================================



Phy/WirelessPhy set RXThresh_ 1.20174e-07
Phy/WirelessPhy set CSThresh_ 1.559e-11
Phy/WirelessPhy set freq_ 9.14e+08
Phy/WirelessPhy set CPThresh_ 10.0
Phy/WirelessPhy set Pt_ 0.281838
Phy/WirelessPhy set L_ 1
Phy/WirelessPhy set Rb_ 2*1e6


Queue/DropTail/PriQueue set Prefer_Routing_Protocols 1

LL set mindelay_ 50us
LL set delay_ 25us
LL set bandwidth_ 0

Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Gt_ 1
Antenna/OmniAntenna set Gr_ 1

Agent/ELLIPSE set limit_boundhole_hop_ 80
Agent/ELLIPSE set alpha_ 0.8
Agent/ELLIPSE set energy_checkpoint_ 995
Agent/ELLIPSE set hello_period_ 0
Agent/ELLIPSE set storage_opt_ 1
Agent/ELLIPSE set range_ 40

Agent/UDP set fid_ 2

Agent/CBR set packetSize_ 50
Agent/CBR set type_ CBR
Agent/CBR set dport_ 0
Agent/CBR set rate_ 0.1Mb
Agent/CBR set sport_ 0
Agent/CBR set interval_ 1

# ======================================================================

#
# Initialize Global Variables
#

# set start time
set startTime [clock seconds]

# set up ns simulator and nam trace
set ns_		[new Simulator]
set chan	[new $opt(chan)]
set prop	[new $opt(prop)]
set topo	[new Topography]

set tracefd	[open $opt(tr) w]
#set namtrace	[open $opt(nam) w]

# run the simulator
$ns_ trace-all $tracefd 
#$ns_ namtrace-all-wireless $namtrace $opt(x) $opt(y) 

$topo load_flatgrid $opt(x) $opt(y) 
$prop topography $topo

set god_ [create-god $opt(nn)]

# configure the nodes
$ns_ node-config -adhocRouting $opt(rp) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 -channel [new $opt(chan)] \
		 -topoInstance $topo \
		 -agentTrace ON \
		 -routerTrace ON \
		 -macTrace OFF \
		 -movementTrace OFF \
		 -energyModel $opt(energymodel) \
		 -idlePower $opt(idlePower) \
		 -rxPower $opt(rxPower) \
		 -txPower $opt(txPower) \
		 -sleepPower $opt(sleepPower) \
		 -transitionPower $opt(transitionPower) \
		 -transitionTime $opt(transitionTime) \
		 -initialEnergy $opt(initialenergy)

# set up nodes
for {set i 0} {$i < $opt(nn)} {incr i} {
	set mnode_($i) [$ns_ node]
}

# set up node position
source ./topo_data.tcl

for {set i 0} {$i < $opt(nn)} { incr i } {
	$ns_ initial_node_pos $mnode_($i) 5
}

# telling nodes when the simulator ends
for {set i 0} {$i < $opt(nn)} {incr i} {
	$ns_ at $opt(stop) "[$mnode_($i) set ragent_] dump"
	$ns_ at $opt(stop).000000001 "$mnode_($i) reset"
}

source ./cbr6.tcl

source ./nodeoff.tcl

# ending nam and the simulation
#$ns_ at $opt(stop) "$ns_ nam-end-wireless $opt(stop)" 
$ns_ at $opt(stop) "stop" 

proc stop {} {
	global ns_ tracefd startTime	;# namtrace
	$ns_ flush-trace
	close $tracefd
	#close $namtrace

	puts "end simulation"

	set runTime [clock second]
	set runTime [expr $runTime - $startTime]

	set s [expr $runTime % 60];	set runTime [expr $runTime / 60];
	set m [expr $runTime % 60];	set runTime [expr $runTime / 60];

	puts "Runtime: $runTime hours, $m minutes, $s seconds"

	$ns_ halt
	exit 0
}

$ns_ run

########### end script #####################
