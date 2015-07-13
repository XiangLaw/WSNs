# node off data
set opt(offn) 0

for {set i 0} {$i < $opt(offn)} {incr i} {
	$ns_ at $offt($i) "$mnode_($offv($i)) off"

	for {set j 0} {$j < $opt(tn)} {incr j} {
		if {$s($j) == $offv($i)} {
			$ns_ at $offt($i) "$cbr_($j) stop"
		}
	}
}
