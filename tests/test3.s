.text
		lui	$t0, 0x1001	#address where array length is stored
		ori	$t1, $t0, 0x4	#address where array begins
		
		addiu	$t2, $0, 5	#length of array = 5
		sw	$t2, 0($t0)
		addiu	$t3, $0, 1
		sw	$t3,  0($t1)
		lw	$t4,  0($t1)
		addiu	$t3, $t3, 1
		sw	$t3,  4($t1)
		lw	$t4,  4($t1)
		addiu	$t3, $t3, 1
		sw	$t3,  8($t1)
		lw	$t4,  8($t1)