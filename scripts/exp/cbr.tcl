# udp data

set opt(tn) 50
set opt(interval_1) 50.0
set opt(interval) 3.0

set s(0)	54	;	set d(0)	1087
set s(1)	26	;	set d(1)	1178
set s(2)	64	;	set d(2)	1244
set s(3)	70	;	set d(3)	1301
set s(4)	121	;	set d(4)	568
set s(5)	129	;	set d(5)	1103
set s(6)	285	;	set d(6)	670
set s(7)	466	;	set d(7)	572
set s(8)	314	;	set d(8)	1305
set s(9)	315	;	set d(9)	1179
set s(10)	357	;	set d(10)	345
set s(11)	358	;	set d(11)	1177
set s(12)	360	;	set d(12)	91
set s(13)	424	;	set d(13)	504
set s(14)	430	;	set d(14)	1302
set s(15)	492	;	set d(15)	1086
set s(16)	521	;	set d(16)	1245
set s(17)	924	;	set d(17)	1306
set s(18)	926	;	set d(18)	679
set s(19)	942	;	set d(19)	569
set s(20)	959	;	set d(20)	390
set s(21)	965	;	set d(21)	1295
set s(22)	957	;	set d(22)	1303
set s(23)	29	;	set d(23)	1182
set s(24)	9	;	set d(24)	936
set s(25)	1248	;	set d(25)	1024
set s(26)	1369	;	set d(26)	1223
set s(27)	1102	;	set d(27)	89
set s(28)	176	;	set d(28)	430
set s(29)	297	;	set d(29)	424
set s(30)	305	;	set d(30)	926
set s(31)	390	;	set d(31)	358
set s(32)	400	;	set d(32)	1116
set s(33)	676	;	set d(33)	492
set s(34)	504	;	set d(34)	64
set s(35)	1377	;	set d(35)	1014
set s(36)	568	;	set d(36)	1113
set s(37)	572	;	set d(37)	424
set s(38)	670	;	set d(38)	520
set s(39)	671	;	set d(39)	124
set s(40)	678	;	set d(40)	360
set s(41)	679	;	set d(41)	1025
set s(42)	680	;	set d(42)	430
set s(43)	681	;	set d(43)	70
set s(44)	683	;	set d(44)	311
set s(45)	1086	;	set d(45)	965
set s(46)	1088	;	set d(46)	70
set s(47)	1089	;	set d(47)	358
set s(48)	1246	;	set d(48)	1115
set s(49)	1181	;	set d(49)	1114

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
