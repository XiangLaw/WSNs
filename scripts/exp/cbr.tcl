# udp data

set opt(tn) 1
set opt(interval_1) 50.0
set opt(interval) 10.0
set opt(cbr_start_1_) 600
set opt(cbr_start_) 800

set s(0)	2569	;	set d(0)	82

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

    $ns_ at [expr $opt(cbr_start_1_) + [expr $i - 1] * $opt(interval_1) / $opt(tn)] "$cbr_($i) start"
    $ns_ at [expr $opt(cbr_start_1_) + 99] "$cbr_($i) stop"

    # Setup a CBR over UDP connection
    set cbr2_($i) [new Application/Traffic/CBR]
    $cbr2_($i) attach-agent $udp_($i)
    $cbr2_($i) set type_ CBR
    $cbr2_($i) set packet_size_ 50
    $cbr2_($i) set rate_ 0.1Mb
    $cbr2_($i) set interval_ $opt(interval)

    $ns_ at [expr $opt(cbr_start_) + [expr $i - 1] * $opt(interval) / $opt(tn)] "$cbr2_($i) start"
    $ns_ at [expr $opt(stop) - 5] "$cbr2_($i) stop"
}
