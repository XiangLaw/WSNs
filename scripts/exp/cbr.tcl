# udp data

set opt(tn) 50
set opt(interval) 10.0

set s(0)	1388	;	set d(0)	90
set s(1)	1264	;	set d(1)	90
set s(2)	1265	;	set d(2)	90
set s(3)	1267	;	set d(3)	90
set s(4)	1268	;	set d(4)	90
set s(5)	1269	;	set d(5)	90
set s(6)	1304	;	set d(6)	90
set s(7)	1305	;	set d(7)	90
set s(8)	1306	;	set d(8)	90
set s(9)	1307	;	set d(9)	90
set s(10)	1308	;	set d(10)	90
set s(11)	1309	;	set d(11)	90
set s(12)	1310	;	set d(12)	90
set s(13)	1311	;	set d(13)	90
set s(14)	1344	;	set d(14)	90
set s(15)	1345	;	set d(15)	90
set s(16)	1346	;	set d(16)	90
set s(17)	1347	;	set d(17)	90
set s(18)	1348	;	set d(18)	90
set s(19)	1349	;	set d(19)	90
set s(20)	1350	;	set d(20)	90
set s(21)	1351	;	set d(21)	90
set s(22)	1383	;	set d(22)	90
set s(23)	1384	;	set d(23)	90
set s(24)	1385	;	set d(24)	90
set s(25)	1386	;	set d(25)	90
set s(26)	1387	;	set d(26)	90
set s(27)	1389	;	set d(27)	90
set s(28)	1390	;	set d(28)	90
set s(29)	1391	;	set d(29)	90
set s(30)	1423	;	set d(30)	90
set s(31)	1424	;	set d(31)	90
set s(32)	1425	;	set d(32)	90
set s(33)	1382	;	set d(33)	90
set s(34)	1422	;	set d(34)	90
set s(35)	1426	;	set d(35)	90
set s(36)	1427	;	set d(36)	90
set s(37)	1428	;	set d(37)	90
set s(38)	1429	;	set d(38)	90
set s(39)	1461	;	set d(39)	90
set s(40)	1462	;	set d(40)	90
set s(41)	1463	;	set d(41)	90
set s(42)	1464	;	set d(42)	90
set s(43)	1465	;	set d(43)	90
set s(44)	1466	;	set d(44)	90
set s(45)	1467	;	set d(45)	90
set s(46)	1468	;	set d(46)	90
set s(47)	1262	;	set d(47)	90
set s(48)	1263	;	set d(48)	90
set s(49)	1460	;	set d(49)	90

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
