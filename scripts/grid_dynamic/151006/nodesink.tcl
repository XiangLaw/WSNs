# node off data
set offv(0) 173
set offv(1) 204
set offv(2) 1590
set offv(3) 1558
set opt(offn) 0

for {set i 0} {$i < $opt(offn)} {incr i} {
	$ns_ at 5 "$mnode_($offv($i)) sink"	
}