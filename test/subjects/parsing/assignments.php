<?php
	$x = $y;
	$x = &$y;

	$x += $y;
	$x -= $y;
	$x *= $y;
	$x /= $y;
	$x %= $y;

	$x .= $y;

	$x &= $y;
	$x |= $y;
	$x ^= $y;

	$x <<= $y;
	$x >>= $y;

	$x++;
	++$x;
	$x--;
	--$x;
?>