# udp data

set opt(tn) 10
set opt(interval) 25.0

set s(0)	87	;	set d(0)	1488
set s(1)	88	;	set d(1)	1408
set s(2)	89	;	set d(2)	1408
set s(3)	90	;	set d(3)	1448
set s(4)	91	;	set d(4)	1325
set s(5)	92	;	set d(5)	1409
set s(6)	128	;	set d(6)	1366
set s(7)	129	;	set d(7)	1488
set s(8)	130	;	set d(8)	1325
set s(9)	131	;	set d(9)	1363

for {set i 0} {$i < $opt(tn)} {incr i} {
	$mnode_($s($i)) setdest [$mnode_($d($i)) set X_] [$mnode_($d($i)) set Y_] 0

	set sink_($i) [new Agent/Null]
	set udp_($i) [new Agent/UDP]	
	$ns_ attach-agent $mnode_($d($i)) $sink_($i)
	$ns_ attach-agent $mnode_($s($i)) $udp_($i)
	$ns_ connect $udp_($i) $sink_($i)
	$udp_($i) set fid_ 2

	#Setup a CBR over UDP connection
	set cbr_($i) [new Application/Traffic/CBR]
	$cbr_($i) attach-agent $udp_($i)
	$cbr_($i) set type_ CBR
	$cbr_($i) set packet_size_ 50
	$cbr_($i) set rate_ 0.1Mb
	$cbr_($i) set interval_ $opt(interval)
	#$cbr set random_ false

	$ns_ at [expr 100 + [expr $i - 1] * $opt(interval) / $opt(tn)] "$cbr_($i) start"
	$ns_ at [expr $opt(stop) - 5] "$cbr_($i) stop"
}
