# udp data

set opt(tn) 50
set opt(first_period_interval) 25.0
set opt(interval) 1 ;# integer, not double

set s(0)	91	;	set d(0)	1313
set s(1)	92	;	set d(1)	1310
set s(2)	90	;	set d(2)	1314
set s(3)	53	;	set d(3)	1277
set s(4)	124	;	set d(4)	1312
set s(5)	1495	;	set d(5)	1349
set s(6)	125	;	set d(6)	1311
set s(7)	54	;	set d(7)	1352
set s(8)	126	;	set d(8)	1350
set s(9)	1235	;	set d(9)	93
set s(10)	1236	;	set d(10)	1495
set s(11)	1237	;	set d(11)	121
set s(12)	1238	;	set d(12)	53
set s(13)	1269	;	set d(13)	93
set s(14)	1272	;	set d(14)	126
set s(15)	1273	;	set d(15)	89
set s(16)	1195	;	set d(16)	57
set s(17)	1270	;	set d(17)	17
set s(18)	1199	;	set d(18)	55
set s(19)	1380	;	set d(19)	1347
set s(20)	1159	;	set d(20)	19
set s(21)	1160	;	set d(21)	94
set s(22)	1161	;	set d(22)	58
set s(23)	1162	;	set d(23)	59
set s(24)	1193	;	set d(24)	92
set s(25)	122	;	set d(25)	1312
set s(26)	123	;	set d(26)	1492
set s(27)	127	;	set d(27)	1310
set s(28)	158	;	set d(28)	1311
set s(29)	159	;	set d(29)	1346
set s(30)	157	;	set d(30)	1312
set s(31)	160	;	set d(31)	1492
set s(32)	161	;	set d(32)	1311
set s(33)	162	;	set d(33)	1312
set s(34)	163	;	set d(34)	1276
set s(35)	1194	;	set d(35)	19
set s(36)	1196	;	set d(36)	1380
set s(37)	1197	;	set d(37)	19
set s(38)	1198	;	set d(38)	57
set s(39)	1232	;	set d(39)	96
set s(40)	164	;	set d(40)	1348
set s(41)	197	;	set d(41)	1492
set s(42)	198	;	set d(42)	1350
set s(43)	199	;	set d(43)	1311
set s(44)	200	;	set d(44)	1309
set s(45)	1233	;	set d(45)	93
set s(46)	1234	;	set d(46)	95
set s(47)	1271	;	set d(47)	91
set s(48)	1274	;	set d(48)	56
set s(49)	201	;	set d(49)	1311

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
	$cbr_($i) set first_period_interval_ $opt(first_period_interval)
	#$cbr set random_ false

	$ns_ at [expr 100 + [expr $i - 1] * $opt(first_period_interval) / $opt(tn)] "$cbr_($i) start"
	$ns_ at 199 "$cbr_($i) stop"
	$ns_ at [expr 200 + [expr $i - 1] * $opt(interval) / $opt(tn)] "$cbr_($i) start"
	$ns_ at [expr $opt(stop) - 5] "$cbr_($i) stop"
}
