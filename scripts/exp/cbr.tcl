# udp data

set opt(tn) 20
set opt(interval_1) 50.0
set opt(interval) 100.0

set s(0)	1086	;	set d(0)	324
set s(1)	1133	;	set d(1)	279
set s(2)	1140	;	set d(2)	309
set s(3)	997	;	set d(3)	505
set s(4)	853	;	set d(4)	597
set s(5)	832	;	set d(5)	354
set s(6)	762	;	set d(6)	584
set s(7)	1053	;	set d(7)	348
set s(8)	352	;	set d(8)	1057
set s(9)	375	;	set d(9)	927
set s(10)	1050	;	set d(10)	319
set s(11)	1094	;	set d(11)	353
set s(12)	1102	;	set d(12)	439
set s(13)	1012	;	set d(13)	454
set s(14)	999	;	set d(14)	401
set s(15)	906	;	set d(15)	554
set s(16)	1048	;	set d(16)	355
set s(17)	1065	;	set d(17)	1470
set s(18)	931	;	set d(18)	665
set s(19)	1479	;	set d(19)	1416

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
