# udp data

set opt(tn) 50
set opt(interval_1) 50.0
set opt(interval) 3.0

set s(0)	1385	;	set d(0)	90
set s(1)	1261	;	set d(1)	90
set s(2)	1262	;	set d(2)	90
set s(3)	1264	;	set d(3)	90
set s(4)	1265	;	set d(4)	90
set s(5)	1266	;	set d(5)	90
set s(6)	1301	;	set d(6)	90
set s(7)	1302	;	set d(7)	90
set s(8)	1303	;	set d(8)	90
set s(9)	1304	;	set d(9)	90
set s(10)	1305	;	set d(10)	90
set s(11)	1306	;	set d(11)	90
set s(12)	1307	;	set d(12)	90
set s(13)	1308	;	set d(13)	90
set s(14)	1341	;	set d(14)	90
set s(15)	1342	;	set d(15)	90
set s(16)	1343	;	set d(16)	90
set s(17)	1344	;	set d(17)	90
set s(18)	1345	;	set d(18)	90
set s(19)	1346	;	set d(19)	90
set s(20)	1347	;	set d(20)	90
set s(21)	1348	;	set d(21)	90
set s(22)	1380	;	set d(22)	90
set s(23)	1381	;	set d(23)	90
set s(24)	1382	;	set d(24)	90
set s(25)	1383	;	set d(25)	90
set s(26)	1384	;	set d(26)	90
set s(27)	1386	;	set d(27)	90
set s(28)	1387	;	set d(28)	90
set s(29)	1388	;	set d(29)	90
set s(30)	1420	;	set d(30)	90
set s(31)	1421	;	set d(31)	90
set s(32)	1422	;	set d(32)	90
set s(33)	1379	;	set d(33)	90
set s(34)	1419	;	set d(34)	90
set s(35)	1423	;	set d(35)	90
set s(36)	1424	;	set d(36)	90
set s(37)	1425	;	set d(37)	90
set s(38)	1426	;	set d(38)	90
set s(39)	1458	;	set d(39)	90
set s(40)	1459	;	set d(40)	90
set s(41)	1460	;	set d(41)	90
set s(42)	1461	;	set d(42)	90
set s(43)	1462	;	set d(43)	90
set s(44)	1463	;	set d(44)	90
set s(45)	1464	;	set d(45)	90
set s(46)	1465	;	set d(46)	90
set s(47)	1259	;	set d(47)	90
set s(48)	1260	;	set d(48)	90
set s(49)	1457	;	set d(49)	90

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
	$cbr_($i) set interval_ $opt(interval_1)
	#$cbr set random_ false

	$ns_ at [expr 100 + [expr $i - 1] * $opt(interval_1) / $opt(tn)] "$cbr_($i) start"
	$ns_ at 199 "$cbr_($i) stop"

	#Setup a CBR over UDP connection
	set cbr2_($i) [new Application/Traffic/CBR]
	$cbr2_($i) attach-agent $udp_($i)
	$cbr2_($i) set type_ CBR
	$cbr2_($i) set packet_size_ 50
	$cbr2_($i) set rate_ 0.1Mb
	$cbr2_($i) set interval_ $opt(interval)

	$ns_ at [expr 200 + [expr $i - 1] * $opt(interval) / $opt(tn)] "$cbr2_($i) start"
	$ns_ at [expr $opt(stop) - 5] "$cbr2_($i) stop"
}