/*
*    Copyright (C) 2024 The University of Tokyo
*    
*    File:          /examples/aes128_hls/src/Sbox_Composite.cpp
*    Project:       sakura-x-shell
*    Author:        Takuya Kojima in The University of Tokyo (tkojima@hal.ipc.i.u-tokyo.ac.jp)
*    Created Date:  14-07-2024 04:11:42
*    Last Modified: 14-07-2024 04:11:42
*/


#include "ap_int.h"

using gf2_t = ap_int<2>;
using gf4_t = ap_int<4>;
using gf8_t = ap_int<8>;

gf8_t AFFINE_TRANSFORM_TABLE[8] = {
	0xF1, // 1111 0001 A7 ^ A6 ^ A5 ^ A4 ^ A0
	0xE3, // 1110 0011 A7 ^ A6 ^ A5 ^ A1 ^ A0
	0xC7, // 1100 0111 A7 ^ A6 ^ A2 ^ A1 ^ A0
	0x8F, // 1000 1111 A7 ^ A3 ^ A2 ^ A1 ^ A0
	0x1F, // 0001 1111 A4 ^ A3 ^ A2 ^ A1 ^ A0
	0x3E, // 0011 1110 A5 ^ A4 ^ A3 ^ A2 ^ A1
	0x7C, // 0111 1100 A6 ^ A5 ^ A4 ^ A3 ^ A2
	0xF8, // 1111 1000 A7 ^ A6 ^ A5 ^ A4 ^ A3
};

gf8_t GF8_ISOMORPHIC_MAP_TABLE[8] = {
	0x43, // 0100 0011 Q6 ^ Q1 ^ Q0
	0x52, // 0101 0010 Q6 ^ Q4 ^ Q1
	0x9E, // 1001 1110 Q7 ^ Q4 ^ Q3 ^ Q2 ^ Q1
	0xC6, // 1100 0110 Q7 ^ Q6 ^ Q2 ^ Q1
	0xAE, // 1010 1110 Q7 ^ Q5 ^ Q3 ^ Q2 ^ Q1
	0xAC, // 1010 1100 Q7 ^ Q5 ^ Q3 ^ Q2
	0xDE, // 1101 1110 Q7 ^ Q6 ^ Q4 ^ Q3 ^ Q2 ^ Q1
	0xA0, // 1010 0000 Q7 ^ Q5
};

gf8_t GF8_INV_ISOMORPHIC_MAP_TABLE[8] = {
	0x75, // 0111 0101 Q6 ^ Q5 ^ Q4 ^ Q2 ^ Q0
	0x30, // 0011 0000 Q5 ^ Q4
	0x9E, // 1001 1110 Q7 ^ Q4 ^ Q3 ^ Q2 ^ Q1
	0x3E, // 0011 1110 Q5 ^ Q4 ^ Q3 ^ Q2 ^ Q1
	0x76, // 0111 0110 Q6 ^ Q5 ^ Q4 ^ Q2 ^ Q1
	0x62, // 0110 0010 Q6 ^ Q5 ^ Q1
	0x44, // 0100 0100 Q6 ^ Q2
	0xE2, // 1110 0010 Q7 ^ Q6 ^ Q5 ^ Q1
};

gf2_t gf2_mult(gf2_t x, gf2_t y)
{
	gf2_t m;
	m[1] = (x[1] & y[1]) ^ (x[0] & y[1]) ^ (x[1] & y[0]);
	m[0] = (x[1] & y[1]) ^ (x[0] & y[0]);
	return m;
}

gf2_t gf2_mult_phi(gf2_t x)
{
	// phi = 0b10
	gf2_t y;
	y[1] = x[0] ^ x[1];
	y[0] = x[1];
	return y;
}



gf8_t gf8_isomorphic_map(gf8_t x)
{
	gf8_t y;
	for (int i = 0; i < 8; i++) {
		#pragma HLS UNROLL
		y[i] = (GF8_ISOMORPHIC_MAP_TABLE[i] & x).xor_reduce();
	}
	return y;
}

gf8_t gf8_inv_isomorphic_map(gf8_t x)
{
	gf8_t y;
	for (int i = 0; i < 8; i++) {
		#pragma HLS UNROLL
		y[i] = (GF8_INV_ISOMORPHIC_MAP_TABLE[i] & x).xor_reduce();
	}
	return y;
}

gf4_t gf4_square(gf4_t x)
{
	gf4_t y;
	y[3] = x[3];
	y[2] = x[3] ^ x[2];
	y[1] = x[2] ^ x[1];
	y[0] = x[3] ^ x[1] ^ x[0];
	return y;
}

gf4_t gf4_mult_lambda(gf4_t x)
{
	// lambda = 0b1100
// /	return ((x[2] ^ x[0]), (x[3] ^ x[2] ^ x[1] ^ x[0]), (x[3]),	(x[2]));
	gf4_t y;
	y[3] = x[2] ^ x[0];
	y[2] = x[3] ^ x[2] ^ x[1] ^ x[0];
	y[1] = x[3];
	y[0] = x[2];
	return y;
}

gf8_t affine_transform(gf8_t x)
{
	gf8_t y;
	for (int i = 0; i < 8; i++) {
		#pragma HLS UNROLL
		y[i] = (AFFINE_TRANSFORM_TABLE[i] & x).xor_reduce();
	}
	return y ^ 0x63;
}

gf4_t gf4_mult(gf4_t x, gf4_t y)
{
	gf2_t x0, x1;
	gf2_t y0, y1;
	(x1, x0) = x;
	(y1, y0) = y;

	gf2_t add0, add1;
	gf2_t mul0, mul1, mul2;

	add0 = x0 ^ x1;
	add1 = y0 ^ y1;

	mul0 = gf2_mult(x1, y1);
	mul1 = gf2_mult(add0, add1);
	mul2 = gf2_mult(x0, y0);

	gf2_t mul0_phi = gf2_mult_phi(mul0);

	gf2_t m1, m0;

	m0 = mul1 ^ mul2;
	m1 = mul0_phi ^ mul2;

	return (m0, m1);

}


gf4_t gf4_mult_inv(gf4_t x)
{
	gf4_t y;

	y[3] = x[3] ^ (x[3] & x[2] & x[1]) ^ (x[3] & x[0]) ^ x[2];
	y[2] = (x[3] & x[2] & x[1]) ^ (x[3] & x[2] & x[0]) ^ (x[3] & x[0]) ^ x[2] ^ (x[2] & x[1]);
	y[1] = x[3] ^ (x[3] & x[2] & x[1]) ^ (x[3] & x[1] & x[0]) ^ x[2] ^ (x[2] & x[0]) ^ x[1];
	y[0] = (x[3] & x[2] & x[1]) ^ (x[3] & x[2] & x[0]) ^ (x[3] & x[1] ) ^ (x[3] & x[1] & x[0]) ^ (x[3] & x[0]) ^ x[2] ^ (x[2] & x[1]) ^ (x[2] & x[1] & x[0]) ^ x[1] ^ x[0];

	return y;
}

void Sbox(unsigned char *in, unsigned char *out)
{
	#pragma HLS INLINE recursive

	// GF(2^8) to GF(((2^2)^2)^2)
	gf4_t g0, g1;
	(g1, g0) = gf8_isomorphic_map(*in);

	// Data flow of multiplicative inverse computation

	gf4_t squared_g1 = gf4_square(g1);

	gf4_t squared_g1_x_lambda = gf4_mult_lambda(squared_g1);

	gf4_t g0_plus_g1 = g0 ^ g1;

	gf4_t g0_plus_g1_x_g0 = gf4_mult(g0_plus_g1, g0);

	gf4_t gf4_inv_in = g0_plus_g1_x_g0 ^ squared_g1_x_lambda;

	gf4_t gf4_inv_out = gf4_mult_inv(gf4_inv_in);

	gf8_t gf8_inv_iso_in = (gf4_mult(gf4_inv_out, g1), gf4_mult(gf4_inv_out, g0_plus_g1));

	// GF(((2^2)^2)^2) to GF(2^8)
	gf8_t gf8_inv_iso_out = gf8_inv_isomorphic_map(gf8_inv_iso_in);

	// apply affine transformation
	*out = affine_transform(gf8_inv_iso_out);

}