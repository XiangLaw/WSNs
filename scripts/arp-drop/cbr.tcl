#Setup a UDP connection
set udp_(29) [new Agent/UDP]
$mnode_(29) setdest 97 112 0
$ns_ attach-agent $mnode_(29) $udp_(29)

set sink_(55) [new Agent/Null]
$ns_ attach-agent $mnode_(55) $sink_(55)

$ns_ connect $udp_(29) $sink_(55)
$udp_(29) set fid_ 2

#Setup a CBR over UDP connection
set cbr_(29) [new Application/Traffic/CBR]
$cbr_(29) attach-agent $udp_(29)
$cbr_(29) set type_ CBR
$cbr_(29) set packet_size_ 50
$cbr_(29) set rate_ 0.1Mb
$cbr_(29) set interval_ 2
#$cbr set random_ false

$ns_ at 100.0 "$cbr_(29) start"
$ns_ at [expr $opt(stop) - 5] "$cbr_(29) stop"

#Setup a UDP connection
set udp_(69) [new Agent/UDP]
$mnode_(69) setdest 82 190 0
$ns_ attach-agent $mnode_(69) $udp_(69)

set sink_(56) [new Agent/Null]
$ns_ attach-agent $mnode_(56) $sink_(56)

$ns_ connect $udp_(69) $sink_(55)
$udp_(69) set fid_ 2

#Setup a CBR over UDP connection
set cbr_(69) [new Application/Traffic/CBR]
$cbr_(69) attach-agent $udp_(69)
$cbr_(69) set type_ CBR
$cbr_(69) set packet_size_ 50
$cbr_(69) set rate_ 0.1Mb
$cbr_(69) set interval_ 2
#$cbr set random_ false

$ns_ at 100.0 "$cbr_(69) start"
$ns_ at [expr $opt(stop) - 5] "$cbr_(69) stop"
