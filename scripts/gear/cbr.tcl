# udp data

set opt(tn) 50
set opt(interval) 25.0

set s(0)	1137	;	set d(0)	58
set s(1)	1012	;	set d(1)	58
set s(2)	1013	;	set d(2)	58
set s(3)	1014	;	set d(3)	58
set s(4)	1015	;	set d(4)	58
set s(5)	1018	;	set d(5)	58
set s(6)	1030	;	set d(6)	58
set s(7)	1031	;	set d(7)	58
set s(8)	1032	;	set d(8)	58
set s(9)	1033	;	set d(9)	58
set s(10)	1034	;	set d(10)	58
set s(11)	1035	;	set d(11)	58
set s(12)	1036	;	set d(12)	58
set s(13)	1037	;	set d(13)	58
set s(14)	1038	;	set d(14)	58
set s(15)	1039	;	set d(15)	58
set s(16)	1048	;	set d(16)	58
set s(17)	1049	;	set d(17)	58
set s(18)	1050	;	set d(18)	58
set s(19)	1051	;	set d(19)	58
set s(20)	1052	;	set d(20)	58
set s(21)	1053	;	set d(21)	58
set s(22)	1054	;	set d(22)	58
set s(23)	1055	;	set d(23)	58
set s(24)	1056	;	set d(24)	58
set s(25)	1057	;	set d(25)	58
set s(26)	1058	;	set d(26)	58
set s(27)	1059	;	set d(27)	58
set s(28)	1060	;	set d(28)	58
set s(29)	1061	;	set d(29)	58
set s(30)	1062	;	set d(30)	58
set s(31)	1064	;	set d(31)	58
set s(32)	1065	;	set d(32)	58
set s(33)	1066	;	set d(33)	58
set s(34)	1067	;	set d(34)	58
set s(35)	1068	;	set d(35)	58
set s(36)	1069	;	set d(36)	58
set s(37)	1070	;	set d(37)	58
set s(38)	1071	;	set d(38)	58
set s(39)	1072	;	set d(39)	58
set s(40)	1073	;	set d(40)	58
set s(41)	1074	;	set d(41)	58
set s(42)	1075	;	set d(42)	58
set s(43)	1076	;	set d(43)	58
set s(44)	1077	;	set d(44)	58
set s(45)	1086	;	set d(45)	58
set s(46)	1087	;	set d(46)	58
set s(47)	1088	;	set d(47)	58
set s(48)	1089	;	set d(48)	58
set s(49)	1090	;	set d(49)	58

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
