set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue	;# for dsdv
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna

set opt(x)		400		;# X dimension of the topography
set opt(y)		400		;# Y dimension of the topography

set opt(ifqlen)		50		;# max packet in ifq
set opt(nn)		90		;# number of nodes
set opt(seed)		0.0
set opt(stop)		250.0		;# simulation time
set opt(tr)		trace.tr		;# trace file
set opt(nam)            nam.out.tr
set opt(rp)             GEAR		;# routing protocol script (dsr or dsdv)
set opt(lm)             "off"		;# log movement

# ======================================================================

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

Agent/Null set sport_		0
Agent/Null set dport_		0

Agent/CBR set sport_		0
Agent/CBR set dport_		0

Agent/TCPSink set sport_	0
Agent/TCPSink set dport_	0

Agent/TCP set sport_		0
Agent/TCP set dport_		0
Agent/TCP set packetSize_	1460

# leecom: ?
Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
#leecom: ?
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/WirelessPhy set CPThresh_ 10.0
Phy/WirelessPhy set CSThresh_ 1.559e-11
Phy/WirelessPhy set RXThresh_ 3.652e-10
Phy/WirelessPhy set Rb_ 2*1e6
Phy/WirelessPhy set freq_ 914e+6 
Phy/WirelessPhy set L_ 1.0


Phy/WirelessPhy set Pt_ 8.5872e-4    ;# 40m

Agent/GEAR set hello_period_ 5
Agent/GEAR set range_ 40
Agent/GEAR set alpha_ 0.8

set opt(energymodel)	 EnergyModel
set opt(radiomodel)      RadioModel
set opt(initialenergy)   5
set opt(idlePower) 	     0.0096
set opt(rxPower) 	     0.045
set opt(txPower) 	     0.0885
set opt(sleepPower) 	 0.000648
set opt(transitionPower) 0.0096
set opt(transitionTime)  0.0129

# ======================================================================


#
# Initialize Global Variables
#

# set up ns simulator and nam trace
set ns_	[new Simulator]
set chan	[new $opt(chan)]
set prop	[new $opt(prop)]
set topo	[new Topography]

set tracefd						[open $opt(tr) w]
set namtrace						[open $opt(nam) w]

# run the simulator
$ns_ trace-all $tracefd 
$ns_ namtrace-all-wireless $namtrace $opt(x) $opt(y) 

# set distances
#set dist(5m)	7.69113e-06
#set dist(9m)	2.37381e-06
#set dist(10m)	1.92278e-06
#set dist(11m)	1.58908e-06
#set dist(12m)	1.33527e-06
#set dist(13m)	1.13774e-06
#set dist(14m)	9.81011e-07
#set dist(15m)	8.54570e-07
#set dist(16m)	7.51087e-07
#set dist(20m)	4.80696e-07
#set dist(25m)	3.07645e-07
#set dist(30m)	2.13643e-07
#set dist(35m)	1.56962e-07
#set dist(40m)	1.20174e-07

# set radius
#Phy/WirelessPhy set CSThresh_ $dist(40m)
#Phy/WirelessPhy set RXThresh_ $dist(40m)

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

source ./cbr.tcl

# ending nam and the simulation
$ns_ at $opt(stop) "$ns_ nam-end-wireless $opt(stop)" 
$ns_ at $opt(stop) "stop" 
$ns_ at [expr $opt(stop) + 0.01] "puts \"end simulation\"; $ns_ halt"

proc stop {} {
	global ns_ tracefd namtrace
	$ns_ flush-trace
	close $tracefd
	close $namtrace
	exit 0
}

$ns_ run

########### end script #####################
