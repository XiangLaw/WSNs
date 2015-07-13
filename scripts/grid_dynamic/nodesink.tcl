# node off data
set offv(0) 212
set offv(1) 240
set offv(2) 1510
set offv(3) 1689
set opt(offn) 4

for {set i 0} {$i < $opt(offn)} {incr i} {
	$ns_ at 5 "$mnode_($offv($i)) sink"	
}