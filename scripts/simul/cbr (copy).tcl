# udp data

set opt(tn) 50
set opt(interval) 50.0

set s(0)	884	;	set d(0)	614
set s(1)	1299	;	set d(1)	1472
set s(2)	33	;	set d(2)	1492
set s(3)	1424	;	set d(3)	1093
set s(4)	560	;	set d(4)	304
set s(5)	820	;	set d(5)	627
set s(6)	363	;	set d(6)	333
set s(7)	1269	;	set d(7)	707
set s(8)	200	;	set d(8)	1219
set s(9)	144	;	set d(9)	866
set s(10)	1082	;	set d(10)	1475
set s(11)	254	;	set d(11)	1129
set s(12)	584	;	set d(12)	1252
set s(13)	165	;	set d(13)	441
set s(14)	873	;	set d(14)	517
set s(15)	1283	;	set d(15)	1351
set s(16)	1390	;	set d(16)	1263
set s(17)	223	;	set d(17)	515
set s(18)	238	;	set d(18)	879
set s(19)	867	;	set d(19)	1074
set s(20)	718	;	set d(20)	419
set s(21)	555	;	set d(21)	215
set s(22)	75	;	set d(22)	226
set s(23)	625	;	set d(23)	1149
set s(24)	480	;	set d(24)	752
set s(25)	411	;	set d(25)	840
set s(26)	208	;	set d(26)	717
set s(27)	1144	;	set d(27)	673
set s(28)	204	;	set d(28)	1371
set s(29)	819	;	set d(29)	50
set s(30)	1216	;	set d(30)	98
set s(31)	989	;	set d(31)	656
set s(32)	156	;	set d(32)	1202
set s(33)	1296	;	set d(33)	110
set s(34)	203	;	set d(34)	390
set s(35)	1035	;	set d(35)	396
set s(36)	1210	;	set d(36)	65
set s(37)	1414	;	set d(37)	599
set s(38)	1290	;	set d(38)	998
set s(39)	1242	;	set d(39)	184
set s(40)	1197	;	set d(40)	1099
set s(41)	660	;	set d(41)	677
set s(42)	724	;	set d(42)	88
set s(43)	1185	;	set d(43)	377
set s(44)	1051	;	set d(44)	949
set s(45)	1078	;	set d(45)	1377
set s(46)	216	;	set d(46)	1091
set s(47)	586	;	set d(47)	410
set s(48)	834	;	set d(48)	687
set s(49)	521	;	set d(49)	817

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
