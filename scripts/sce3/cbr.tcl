# udp data

set opt(tn) 86
set opt(interval) 30.0

set s(0)	284	;	set d(0)	928
set s(1)	285	;	set d(1)	1116
set s(2)	286	;	set d(2)	1040
set s(3)	319	;	set d(3)	927
set s(4)	320	;	set d(4)	1046
set s(5)	321	;	set d(5)	1450
set s(6)	322	;	set d(6)	930
set s(7)	323	;	set d(7)	969
set s(8)	324	;	set d(8)	1124
set s(9)	325	;	set d(9)	1078
set s(10)	326	;	set d(10)	1159
set s(11)	356	;	set d(11)	1081
set s(12)	357	;	set d(12)	1450
set s(13)	358	;	set d(13)	1117
set s(14)	359	;	set d(14)	1081
set s(15)	360	;	set d(15)	1006
set s(16)	361	;	set d(16)	1040
set s(17)	362	;	set d(17)	1008
set s(18)	363	;	set d(18)	926
set s(19)	364	;	set d(19)	1077
set s(20)	365	;	set d(20)	1003
set s(21)	393	;	set d(21)	972
set s(22)	394	;	set d(22)	1044
set s(23)	395	;	set d(23)	964
set s(24)	396	;	set d(24)	930
set s(25)	397	;	set d(25)	972
set s(26)	398	;	set d(26)	1043
set s(27)	399	;	set d(27)	929
set s(28)	400	;	set d(28)	1003
set s(29)	401	;	set d(29)	1082
set s(30)	402	;	set d(30)	1045
set s(31)	403	;	set d(31)	1040
set s(32)	431	;	set d(32)	893
set s(33)	432	;	set d(33)	1083
set s(34)	433	;	set d(34)	1040
set s(35)	434	;	set d(35)	1159
set s(36)	435	;	set d(36)	1047
set s(37)	436	;	set d(37)	1079
set s(38)	437	;	set d(38)	1009
set s(39)	438	;	set d(39)	1039
set s(40)	439	;	set d(40)	971
set s(41)	440	;	set d(41)	1116
set s(42)	441	;	set d(42)	1040
set s(43)	470	;	set d(43)	1086
set s(44)	471	;	set d(44)	1044
set s(45)	472	;	set d(45)	1160
set s(46)	473	;	set d(46)	932
set s(47)	474	;	set d(47)	969
set s(48)	475	;	set d(48)	1158
set s(49)	476	;	set d(49)	1121
set s(50)	477	;	set d(50)	1005
set s(51)	478	;	set d(51)	1040
set s(52)	509	;	set d(52)	932
set s(53)	510	;	set d(53)	1043
set s(54)	511	;	set d(54)	1045
set s(55)	512	;	set d(55)	1002
set s(56)	513	;	set d(56)	1121
set s(57)	514	;	set d(57)	969
set s(58)	515	;	set d(58)	1006
set s(59)	548	;	set d(59)	1086
set s(60)	638	;	set d(60)	691
set s(61)	639	;	set d(61)	804
set s(62)	674	;	set d(62)	618
set s(63)	675	;	set d(63)	615
set s(64)	676	;	set d(64)	727
set s(65)	677	;	set d(65)	727
set s(66)	678	;	set d(66)	727
set s(67)	711	;	set d(67)	730
set s(68)	712	;	set d(68)	767
set s(69)	713	;	set d(69)	656
set s(70)	714	;	set d(70)	617
set s(71)	715	;	set d(71)	765
set s(72)	716	;	set d(72)	618
set s(73)	749	;	set d(73)	768
set s(74)	750	;	set d(74)	732
set s(75)	751	;	set d(75)	656
set s(76)	752	;	set d(76)	580
set s(77)	753	;	set d(77)	692
set s(78)	787	;	set d(78)	804
set s(79)	788	;	set d(79)	727
set s(80)	789	;	set d(80)	651
set s(81)	790	;	set d(81)	767
set s(82)	791	;	set d(82)	733
set s(83)	826	;	set d(83)	652
set s(84)	827	;	set d(84)	618
set s(85)	1495	;	set d(85)	694

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
