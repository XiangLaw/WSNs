set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue	;# for dsdv
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna

set opt(x)		1000		;# X dimension of the topography
set opt(y)		1000		;# Y dimension of the topography

set opt(ifqlen)		50		;# max packet in ifq
set opt(nn)		1500		;# number of nodes
set opt(seed)		0.0
set opt(stop)		500.0		;# simulation time
set opt(tr)		trace.tr	;# trace file
set opt(nam)            nam.out.tr
set opt(rp)             ELBARGRIDOFFLINE		;# routing protocol script (dsr or dsdv)
set opt(lm)             "off"		;# log movement

set opt(energymodel)     EnergyModel    ;
set opt(radiomodel)    	 RadioModel     ;
set opt(initialenergy)   1000		;# Initial energy in Joules
set opt(idlePower) 	 0.0096
set opt(rxPower) 	 0.045
set opt(txPower) 	 0.0885
set opt(sleepPower) 	 0.000648
set opt(transitionPower) 0.024
set opt(transitionTime)  0.0129

# ======================================================================

LL set mindelay_	50us
LL set delay_		25us
LL set bandwidth_	0	;# not used

Agent/Null set sport_	0
Agent/Null set dport_	0

Agent/CBR set sport_	0
Agent/CBR set dport_	0

# leecom: ?
Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
#leecom: ?
Antenna/OmniAntenna set X_ 	0
Antenna/OmniAntenna set Y_ 	0
Antenna/OmniAntenna set Z_ 	1.5
Antenna/OmniAntenna set Gt_ 	1.0
Antenna/OmniAntenna set Gr_ 	1.0

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/WirelessPhy set CPThresh_ 	10.0
Phy/WirelessPhy set CSThresh_ 	1.559e-11
Phy/WirelessPhy set RXThresh_ 	3.652e-10
Phy/WirelessPhy set Rb_ 	2*1e6
Phy/WirelessPhy set freq_ 	914e+6 
Phy/WirelessPhy set L_ 		1.0
Phy/WirelessPhy set Pt_ 	8.5872e-4    ;# 40m

# Initialize the ELBARGRIDOFFLINE Routing Agent

Agent/ELBARGRIDOFFLINE set hello_period_ 500
Agent/ELBARGRIDOFFLINE set range_ 40
Agent/ELBARGRIDOFFLINE set broadcast_rate_ 1
Agent/ELBARGRIDOFFLINE set storage_opt_ 2

#define ALL_HOLE 0		// store all hole's information in all node in boundary
#define ADJ_NODE 1		// store next node in hole's information in all node in boundary
#define ONE_NODE 2		// store all hole's information only in control node

# ======================================================================

#
# Initialize Global Variables
#

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
	$ns_ at $opt(stop).000000001 "$mnode_($i) reset"
}

source ./cbr.tcl

# ending nam and the simulation
#$ns_ at $opt(stop) "$ns_ nam-end-wireless $opt(stop)" 
$ns_ at $opt(stop) "stop" 

proc stop {} {
	#global ns_ tracefd ;#namtrace
	global ns_ tracefd namtrace
	$ns_ flush-trace
	close $tracefd
#	close $namtrace
	
	puts "end simulation"; 
	$ns_ halt
	exit 0
}

$ns_ run

########### end script #####################
