# udp data

set opt(tn) 50
set opt(interval_1) 50.0
set opt(interval) 3.0

set s(0)	1338	;	set d(0)	100
set s(1)	1299	;	set d(1)	183
set s(2)	1302	;	set d(2)	221
set s(3)	1304	;	set d(3)	181
set s(4)	1335	;	set d(4)	222
set s(5)	1340	;	set d(5)	181
set s(6)	1341	;	set d(6)	220
set s(7)	1342	;	set d(7)	184
set s(8)	1343	;	set d(8)	98
set s(9)	1344	;	set d(9)	138
set s(10)	1376	;	set d(10)	259
set s(11)	1380	;	set d(11)	218
set s(12)	1381	;	set d(12)	222
set s(13)	1382	;	set d(13)	221
set s(14)	1383	;	set d(14)	140
set s(15)	1384	;	set d(15)	98
set s(16)	1385	;	set d(16)	259
set s(17)	1418	;	set d(17)	181
set s(18)	1419	;	set d(18)	181
set s(19)	1420	;	set d(19)	184
set s(20)	1421	;	set d(20)	98
set s(21)	1422	;	set d(21)	263
set s(22)	1218	;	set d(22)	224
set s(23)	1223	;	set d(23)	183
set s(24)	1225	;	set d(24)	139
set s(25)	1257	;	set d(25)	180
set s(26)	1259	;	set d(26)	223
set s(27)	1262	;	set d(27)	99
set s(28)	1379	;	set d(28)	182
set s(29)	1423	;	set d(29)	262
set s(30)	1301	;	set d(30)	219
set s(31)	1303	;	set d(31)	179
set s(32)	1345	;	set d(32)	178
set s(33)	1414	;	set d(33)	260
set s(34)	1306	;	set d(34)	58
set s(35)	1417	;	set d(35)	141
set s(36)	1416	;	set d(36)	62
set s(37)	1415	;	set d(37)	63
set s(38)	1377	;	set d(38)	103
set s(39)	1260	;	set d(39)	144
set s(40)	1258	;	set d(40)	142
set s(41)	1375	;	set d(41)	65
set s(42)	1336	;	set d(42)	102
set s(43)	1298	;	set d(43)	57
set s(44)	1378	;	set d(44)	137
set s(45)	1339	;	set d(45)	17
set s(46)	1305	;	set d(46)	177
set s(47)	1424	;	set d(47)	97
set s(48)	1300	;	set d(48)	60
set s(49)	1337	;	set d(49)	101

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

    # Setup a CBR over UDP connection
    set cbr2_($i) [new Application/Traffic/CBR]
    $cbr2_($i) attach-agent $udp_($i)
    $cbr2_($i) set type_ CBR
    $cbr2_($i) set packet_size_ 50
    $cbr2_($i) set rate_ 0.1Mb
    $cbr2_($i) set interval_ $opt(interval)

    $ns_ at [expr 200 + [expr $i - 1] * $opt(interval) / $opt(tn)] "$cbr2_($i) start"
    $ns_ at [expr $opt(stop) - 5] "$cbr2_($i) stop"
}
