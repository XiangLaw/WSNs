# udp data

set opt(tn) 50
set opt(interval) 10.0

set s(0)	1351	;	set d(0)	60
set s(1)	1265	;	set d(1)	58
set s(2)	1266	;	set d(2)	52
set s(3)	1267	;	set d(3)	14
set s(4)	1268	;	set d(4)	19
set s(5)	1269	;	set d(5)	14
set s(6)	1270	;	set d(6)	16
set s(7)	1271	;	set d(7)	18
set s(8)	1272	;	set d(8)	15
set s(9)	1273	;	set d(9)	54
set s(10)	1274	;	set d(10)	1380
set s(11)	1275	;	set d(11)	92
set s(12)	1299	;	set d(12)	91
set s(13)	1300	;	set d(13)	56
set s(14)	1301	;	set d(14)	59
set s(15)	12	;	set d(15)	1346
set s(16)	13	;	set d(16)	1299
set s(17)	14	;	set d(17)	1310
set s(18)	15	;	set d(18)	1309
set s(19)	16	;	set d(19)	1270
set s(20)	17	;	set d(20)	1314
set s(21)	18	;	set d(21)	1305
set s(22)	19	;	set d(22)	1340
set s(23)	20	;	set d(23)	1314
set s(24)	21	;	set d(24)	1305
set s(25)	23	;	set d(25)	1349
set s(26)	52	;	set d(26)	1273
set s(27)	53	;	set d(27)	1341
set s(28)	54	;	set d(28)	1272
set s(29)	4	;	set d(29)	1244
set s(30)	5	;	set d(30)	1320
set s(31)	6	;	set d(31)	1362
set s(32)	7	;	set d(32)	1323
set s(33)	8	;	set d(33)	1317
set s(34)	9	;	set d(34)	1317
set s(35)	10	;	set d(35)	1320
set s(36)	40	;	set d(36)	1251
set s(37)	41	;	set d(37)	1243
set s(38)	42	;	set d(38)	1244
set s(39)	1243	;	set d(39)	84
set s(40)	1244	;	set d(40)	81
set s(41)	1246	;	set d(41)	6
set s(42)	1247	;	set d(42)	88
set s(43)	1248	;	set d(43)	4
set s(44)	1249	;	set d(44)	10
set s(45)	1250	;	set d(45)	50
set s(46)	1251	;	set d(46)	49
set s(47)	1278	;	set d(47)	113
set s(48)	22	;	set d(48)	1347
set s(49)	1350	;	set d(49)	96

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
