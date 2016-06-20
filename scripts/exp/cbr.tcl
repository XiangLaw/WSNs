# udp data

set opt(tn) 50
set opt(interval_1) 50.0
set opt(interval) 50.0

set s(0)	1312	;	set d(0)	22
set s(1)	1264	;	set d(1)	222
set s(2)	1265	;	set d(2)	105
set s(3)	1266	;	set d(3)	107
set s(4)	1267	;	set d(4)	176
set s(5)	1268	;	set d(5)	24
set s(6)	1269	;	set d(6)	144
set s(7)	1270	;	set d(7)	100
set s(8)	1271	;	set d(8)	175
set s(9)	1272	;	set d(9)	57
set s(10)	1263	;	set d(10)	181
set s(11)	1341	;	set d(11)	102
set s(12)	1301	;	set d(12)	141
set s(13)	1302	;	set d(13)	26
set s(14)	1303	;	set d(14)	182
set s(15)	1304	;	set d(15)	138
set s(16)	1305	;	set d(16)	177
set s(17)	1306	;	set d(17)	108
set s(18)	1307	;	set d(18)	135
set s(19)	1308	;	set d(19)	23
set s(20)	1309	;	set d(20)	140
set s(21)	1310	;	set d(21)	139
set s(22)	1311	;	set d(22)	59
set s(23)	1313	;	set d(23)	21
set s(24)	1340	;	set d(24)	143
set s(25)	1342	;	set d(25)	66
set s(26)	1343	;	set d(26)	67
set s(27)	1344	;	set d(27)	178
set s(28)	1345	;	set d(28)	104
set s(29)	1346	;	set d(29)	142
set s(30)	1347	;	set d(30)	184
set s(31)	1348	;	set d(31)	101
set s(32)	1349	;	set d(32)	103
set s(33)	1350	;	set d(33)	20
set s(34)	1352	;	set d(34)	96
set s(35)	1379	;	set d(35)	146
set s(36)	1380	;	set d(36)	106
set s(37)	1381	;	set d(37)	183
set s(38)	1382	;	set d(38)	61
set s(39)	1383	;	set d(39)	62
set s(40)	1384	;	set d(40)	18
set s(41)	1385	;	set d(41)	179
set s(42)	1386	;	set d(42)	58
set s(43)	1387	;	set d(43)	25
set s(44)	1388	;	set d(44)	63
set s(45)	1389	;	set d(45)	64
set s(46)	1390	;	set d(46)	65
set s(47)	1391	;	set d(47)	99
set s(48)	1392	;	set d(48)	19
set s(49)	1351	;	set d(49)	60

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
