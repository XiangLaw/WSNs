# Script for WisSim simulator. Last edit 09/07/2016 20:11:06

set opt(x)	1000	;# X dimension of the topography
set opt(y)	1000	;# Y dimension of the topography
set opt(stop)	500	;# simulation time
set opt(nn)	1702	;# number of nodes
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
set opt(rp)	CORBAL
set opt(trans)	UDP
set opt(apps)	CBR

set opt(energymodel)	 EnergyModel
set opt(radiomodel)      RadioModel
set opt(initialenergy)   1000
set opt(idlePower) 	     0.0096
set opt(rxPower) 	     0.045
set opt(txPower) 	     0.0885
set opt(sleepPower) 	 0.000648
set opt(transitionPower) 0.0096
set opt(transitionTime)  0.0129

# ======================================================================



Phy/WirelessPhy set RXThresh_ 3.66152e-10
Phy/WirelessPhy set CSThresh_ 3.66152e-10
Phy/WirelessPhy set freq_ 9.14e+08
Phy/WirelessPhy set CPThresh_ 10.0
Phy/WirelessPhy set Pt_ 8.5872e-4
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

Agent/CORBAL set energy_checkpoint_ 995
Agent/CORBAL set hello_period_ 0
Agent/CORBAL set range_ 40
Agent/CORBAL set limit_boundhole_hop_ 80
Agent/CORBAL set min_boundhole_hop_ 5
Agent/CORBAL set n_ 8
Agent/CORBAL set k_n_ 3
Agent/CORBAL set epsilon_ 1.2
Agent/CORBAL set net_width_ 1000
Agent/CORBAL set net_height_ 1000

Agent/UDP set fid_ 2

Agent/CBR set packetSize_ 50
Agent/CBR set type_ CBR
Agent/CBR set dport_ 0
Agent/CBR set rate_ 0.1Mb
Agent/CBR set sport_ 0
Agent/CBR set interval_1_ 50.0
Agent/CBR set interval_ 5.0

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

puts "Routing Protocol: $opt(rp)"

# set up nodes
for {set i 0} {$i < $opt(nn)} {incr i} {
	set mnode_($i) [$ns_ node]
}
$defaultRNG seed 0

# set up node position
source ./topo_data.tcl

for {set i 0} {$i < $opt(nn)} { incr i } {
	$ns_ initial_node_pos $mnode_($i) 5
}

# telling nodes when the simulator ends
for {set i 0} {$i < $opt(nn)} {incr i} {
	$ns_ at [expr $opt(stop) - 0.000000001] "$mnode_($i) off"
	$ns_ at $opt(stop) "[$mnode_($i) set ragent_] dump"
	$ns_ at $opt(stop).000000001 "$mnode_($i) reset"
}

source ./cbr.tcl

source ./nodeoff.tcl

source ./nodesink.tcl

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
