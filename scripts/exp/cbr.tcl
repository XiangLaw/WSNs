# udp data

set opt(tn) 50
set opt(interval_1) 100.0
set opt(interval) 100.0

set s(0)	1295	;	set d(0)	793
set s(1)	1339	;	set d(1)	1212
set s(2)	1403	;	set d(2)	1316
set s(3)	1379	;	set d(3)	1309
set s(4)	265	;	set d(4)	836
set s(5)	429	;	set d(5)	585
set s(6)	769	;	set d(6)	295
set s(7)	1368	;	set d(7)	1398
set s(8)	220	;	set d(8)	622
set s(9)	811	;	set d(9)	333
set s(10)	920	;	set d(10)	61
set s(11)	1259	;	set d(11)	668
set s(12)	1137	;	set d(12)	1196
set s(13)	1023	;	set d(13)	473
set s(14)	672	;	set d(14)	412
set s(15)	366	;	set d(15)	887
set s(16)	616	;	set d(16)	1436
set s(17)	1248	;	set d(17)	1362
set s(18)	218	;	set d(18)	1042
set s(19)	537	;	set d(19)	590
set s(20)	493	;	set d(20)	627
set s(21)	173	;	set d(21)	1128
set s(22)	1113	;	set d(22)	150
set s(23)	1355	;	set d(23)	1197
set s(24)	1408	;	set d(24)	1271
set s(25)	1167	;	set d(25)	470
set s(26)	754	;	set d(26)	374
set s(27)	257	;	set d(27)	760
set s(28)	625	;	set d(28)	664
set s(29)	862	;	set d(29)	657
set s(30)	346	;	set d(30)	565
set s(31)	1308	;	set d(31)	1402
set s(32)	1220	;	set d(32)	1332
set s(33)	1229	;	set d(33)	1182
set s(34)	1301	;	set d(34)	1338
set s(35)	1185	;	set d(35)	1219
set s(36)	1357	;	set d(36)	1302
set s(37)	1407	;	set d(37)	1123
set s(38)	432	;	set d(38)	1438
set s(39)	502	;	set d(39)	656
set s(40)	97	;	set d(40)	775
set s(41)	233	;	set d(41)	872
set s(42)	291	;	set d(42)	621
set s(43)	634	;	set d(43)	403
set s(44)	1158	;	set d(44)	1289
set s(45)	1188	;	set d(45)	1327
set s(46)	1356	;	set d(46)	372
set s(47)	1009	;	set d(47)	223
set s(48)	737	;	set d(48)	1330
set s(49)	165	;	set d(49)	1279

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
