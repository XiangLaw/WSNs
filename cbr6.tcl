# udp data

set opt(tn) 40
set opt(interval) 50.0

set s(0)	1145	;	set d(0)	162
set s(1)	767	;	set d(1)	1302
set s(2)	36	;	set d(2)	415
set s(3)	922	;	set d(3)	491
set s(4)	1079	;	set d(4)	1234
set s(5)	1196	;	set d(5)	1031
set s(6)	822	;	set d(6)	1165
set s(7)	479	;	set d(7)	172
set s(8)	628	;	set d(8)	616
set s(9)	371	;	set d(9)	205
set s(10)	1375	;	set d(10)	20
set s(11)	1241	;	set d(11)	1141
set s(12)	322	;	set d(12)	558
set s(13)	38	;	set d(13)	1207
set s(14)	126	;	set d(14)	456
set s(15)	320	;	set d(15)	99
set s(16)	1065	;	set d(16)	825
set s(17)	202	;	set d(17)	284
set s(18)	135	;	set d(18)	83
set s(19)	417	;	set d(19)	829
set s(20)	823	;	set d(20)	676
set s(21)	510	;	set d(21)	60
set s(22)	1160	;	set d(22)	1041
set s(23)	416	;	set d(23)	1461
set s(24)	1058	;	set d(24)	990
set s(25)	152	;	set d(25)	105
set s(26)	121	;	set d(26)	338
set s(27)	139	;	set d(27)	945
set s(28)	553	;	set d(28)	1305
set s(29)	738	;	set d(29)	996
set s(30)	739	;	set d(30)	258
set s(31)	271	;	set d(31)	989
set s(32)	306	;	set d(32)	12
set s(33)	911	;	set d(33)	748
set s(34)	94	;	set d(34)	614
set s(35)	810	;	set d(35)	1061
set s(36)	952	;	set d(36)	1108
set s(37)	928	;	set d(37)	326
set s(38)	1159	;	set d(38)	1314
set s(39)	1311	;	set d(39)	1062
set s(40)	238	;	set d(40)	211
set s(41)	943	;	set d(41)	115
set s(42)	667	;	set d(42)	811
set s(43)	1175	;	set d(43)	1247
set s(44)	86	;	set d(44)	727
set s(45)	955	;	set d(45)	969
set s(46)	1232	;	set d(46)	266
set s(47)	649	;	set d(47)	318
set s(48)	800	;	set d(48)	315
set s(49)	876	;	set d(49)	608

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
