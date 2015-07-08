# node off data
set offv(0) 230
set offv(1) 259
set offv(2) 262
set offv(3) 263
set offv(4) 287
set offv(5) 290
set opt(offn) 6

for {set i 0} {$i < $opt(offn)} {incr i} {
	$ns_ at 5 "$mnode_($offv($i)) sink"	
}