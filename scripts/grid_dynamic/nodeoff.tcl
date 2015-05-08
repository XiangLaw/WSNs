# node off data
set offv(0) 230	;	set offt(0) 494.189025469943
set offv(1) 259	;	set offt(1) 405.563545617033
set offv(2) 262	;	set offt(2) 430.91741964402
set offv(3) 263	;	set offt(3) 465.274149638797
set offv(4) 287	;	set offt(4) 478.296287396861
set offv(5) 290	;	set offt(5) 378.579911454932
set offv(6) 293	;	set offt(6) 383.520990310299
set offv(7) 316	;	set offt(7) 462.960605250106
set offv(8) 317	;	set offt(8) 435.542395690439
set offv(9) 318	;	set offt(9) 360.102965869463
set offv(10) 321	;	set offt(10) 269.457353762449
set offv(11) 322	;	set offt(11) 259.966155532006
set offv(12) 324	;	set offt(12) 312.188883141461
set offv(13) 347	;	set offt(13) 404.526666718038
set offv(14) 348	;	set offt(14) 328.654763360304
set offv(15) 349	;	set offt(15) 319.939026583871
set offv(16) 350	;	set offt(16) 259.471832263455
set offv(17) 351	;	set offt(17) 308.88156093979
set offv(18) 352	;	set offt(18) 211.21162809475
set offv(19) 353	;	set offt(19) 170.05339895254
set offv(20) 354	;	set offt(20) 191.159490754584
set offv(21) 355	;	set offt(21) 347.543889015324
set offv(22) 356	;	set offt(22) 414.639254453982
set offv(23) 381	;	set offt(23) 169.922843876133
set offv(24) 382	;	set offt(24) 141.492096225425
set offv(25) 383	;	set offt(25) 70.5979498078254
set offv(26) 384	;	set offt(26) 156.945710245746
set offv(27) 385	;	set offt(27) 144.539890465614
set offv(28) 386	;	set offt(28) 282.08622611199
set offv(29) 409	;	set offt(29) 380.078149201449
set offv(30) 410	;	set offt(30) 355.723973520626
set offv(31) 411	;	set offt(31) 292.157210975003
set offv(32) 412	;	set offt(32) 216.077099259365
set offv(33) 413	;	set offt(33) 82.4804881580733
set offv(34) 414	;	set offt(34) 51.4180072322444
set offv(35) 415	;	set offt(35) 103.417110115802
set offv(36) 416	;	set offt(36) 104.710284736123
set offv(37) 417	;	set offt(37) 269.067539126605
set offv(38) 420	;	set offt(38) 486.85438360993
set offv(39) 441	;	set offt(39) 496.616119145427
set offv(40) 442	;	set offt(40) 380.766395218264
set offv(41) 443	;	set offt(41) 207.201010780142
set offv(42) 444	;	set offt(42) 175.872521130115
set offv(43) 445	;	set offt(43) 133.283759206003
set offv(44) 446	;	set offt(44) 191.195481741271
set offv(45) 447	;	set offt(45) 149.205790152333
set offv(46) 448	;	set offt(46) 293.535128536451
set offv(47) 449	;	set offt(47) 313.451628311989
set offv(48) 450	;	set offt(48) 413.426880754174
set offv(49) 472	;	set offt(49) 411.333071810294
set offv(50) 473	;	set offt(50) 292.690321575384
set offv(51) 474	;	set offt(51) 225.299946321848
set offv(52) 475	;	set offt(52) 262.806799128589
set offv(53) 476	;	set offt(53) 214.768231587393
set offv(54) 477	;	set offt(54) 170.710754734834
set offv(55) 478	;	set offt(55) 227.685304510243
set offv(56) 479	;	set offt(56) 478.913839657421
set offv(57) 481	;	set offt(57) 467.255364767148
set offv(58) 504	;	set offt(58) 382.244016417758
set offv(59) 506	;	set offt(59) 362.487329025855
set offv(60) 507	;	set offt(60) 340.32339649998
set offv(61) 508	;	set offt(61) 224.230882401253
set offv(62) 509	;	set offt(62) 284.182825447774
set offv(63) 512	;	set offt(63) 478.01550581321
set offv(64) 534	;	set offt(64) 421.933714489786
set offv(65) 537	;	set offt(65) 388.520803789986
set offv(66) 538	;	set offt(66) 424.273024919457
set offv(67) 540	;	set offt(67) 393.513654270671
set offv(68) 571	;	set offt(68) 447.183909372752
set offv(69) 969	;	set offt(69) 414.550127459059
set offv(70) 1010	;	set offt(70) 214.471386081533
set opt(offn) 71

for {set i 0} {$i < $opt(offn)} {incr i} {
	$ns_ at $offt($i) "$mnode_($offv($i)) off"

	for {set j 0} {$j < $opt(tn)} {incr j} {
		if {$s($j) == $offv($i)} {
			$ns_ at $offt($i) "$cbr_($j) stop"
		}
	}
}
