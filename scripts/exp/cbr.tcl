# udp data

set opt(tn) 60
set opt(interval_1) 50.0
set opt(interval) 100.0

set s(0)	849	;	set d(0)	530
set s(1)	952	;	set d(1)	490
set s(2)	1030	;	set d(2)	453
set s(3)	995	;	set d(3)	1483
set s(4)	925	;	set d(4)	519
set s(5)	859	;	set d(5)	664
set s(6)	763	;	set d(6)	1502
set s(7)	701	;	set d(7)	782
set s(8)	1492	;	set d(8)	879
set s(9)	494	;	set d(9)	953
set s(10)	459	;	set d(10)	1029
set s(11)	420	;	set d(11)	1070
set s(12)	378	;	set d(12)	1035
set s(13)	413	;	set d(13)	1530
set s(14)	553	;	set d(14)	927
set s(15)	689	;	set d(15)	1439
set s(16)	878	;	set d(16)	674
set s(17)	844	;	set d(17)	643
set s(18)	985	;	set d(18)	1482
set s(19)	1064	;	set d(19)	457
set s(20)	1108	;	set d(20)	454
set s(21)	1072	;	set d(21)	484
set s(22)	999	;	set d(22)	629
set s(23)	894	;	set d(23)	554
set s(24)	931	;	set d(24)	482
set s(25)	1157	;	set d(25)	1486
set s(26)	1007	;	set d(26)	586
set s(27)	979	;	set d(27)	604
set s(28)	1168	;	set d(28)	639
set s(29)	1282	;	set d(29)	458
set s(30)	1290	;	set d(30)	159
set s(31)	1335	;	set d(31)	520
set s(32)	1527	;	set d(32)	450
set s(33)	1233	;	set d(33)	628
set s(34)	864	;	set d(34)	692
set s(35)	1542	;	set d(35)	750
set s(36)	576	;	set d(36)	814
set s(37)	424	;	set d(37)	1063
set s(38)	1466	;	set d(38)	1180
set s(39)	194	;	set d(39)	1186
set s(40)	258	;	set d(40)	1150
set s(41)	370	;	set d(41)	1002
set s(42)	438	;	set d(42)	1001
set s(43)	1499	;	set d(43)	761
set s(44)	621	;	set d(44)	1437
set s(45)	742	;	set d(45)	825
set s(46)	977	;	set d(46)	673
set s(47)	1129	;	set d(47)	605
set s(48)	1244	;	set d(48)	698
set s(49)	1323	;	set d(49)	525
set s(50)	1327	;	set d(50)	526
set s(51)	1371	;	set d(51)	558
set s(52)	1304	;	set d(52)	518
set s(53)	1198	;	set d(53)	590
set s(54)	281	;	set d(54)	847
set s(55)	1459	;	set d(55)	987
set s(56)	99	;	set d(56)	1104
set s(57)	55	;	set d(57)	1068
set s(58)	117	;	set d(58)	1113
set s(59)	145	;	set d(59)	960

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
