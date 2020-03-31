


static void
curve25519_pow_two5mtwo0_two250mtwo0(bignum25519 b) {
	bignum25519 ALIGN(16) t0,c;

	 
	 curve25519_square_times(t0, b, 5);
	 curve25519_mul_noinline(b, t0, b);
	 curve25519_square_times(t0, b, 10);
	 curve25519_mul_noinline(c, t0, b);
	 curve25519_square_times(t0, c, 20);
	 curve25519_mul_noinline(t0, t0, c);
	 curve25519_square_times(t0, t0, 10);
	 curve25519_mul_noinline(b, t0, b);
	 curve25519_square_times(t0, b, 50);
	 curve25519_mul_noinline(c, t0, b);
	 curve25519_square_times(t0, c, 100);
	 curve25519_mul_noinline(t0, t0, c);
	 curve25519_square_times(t0, t0, 50);
	 curve25519_mul_noinline(b, t0, b);
}


static void
curve25519_recip(bignum25519 out, const bignum25519 z) {
	bignum25519 ALIGN(16) a,t0,b;

	 curve25519_square_times(a, z, 1); 
	 curve25519_square_times(t0, a, 2);
	 curve25519_mul_noinline(b, t0, z); 
	 curve25519_mul_noinline(a, b, a); 
	 curve25519_square_times(t0, a, 1);
	 curve25519_mul_noinline(b, t0, b);
	 curve25519_pow_two5mtwo0_two250mtwo0(b);
	 curve25519_square_times(b, b, 5);
	 curve25519_mul_noinline(out, b, a);
}


static void
curve25519_pow_two252m3(bignum25519 two252m3, const bignum25519 z) {
	bignum25519 ALIGN(16) b,c,t0;

	 curve25519_square_times(c, z, 1); 
	 curve25519_square_times(t0, c, 2); 
	 curve25519_mul_noinline(b, t0, z); 
	 curve25519_mul_noinline(c, b, c); 
	 curve25519_square_times(t0, c, 1);
	 curve25519_mul_noinline(b, t0, b);
	 curve25519_pow_two5mtwo0_two250mtwo0(b);
	 curve25519_square_times(b, b, 2);
	 curve25519_mul_noinline(two252m3, b, z);
}








