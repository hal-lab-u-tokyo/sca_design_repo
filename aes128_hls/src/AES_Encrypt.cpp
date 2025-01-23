/*
*    Copyright (C) 2024 The University of Tokyo
*    
*    File:          /examples/aes128_hls/src/AES_Encrypt.cpp
*    Project:       sakura-x-shell
*    Author:        Takuya Kojima in The University of Tokyo (tkojima@hal.ipc.i.u-tokyo.ac.jp)
*    Created Date:  14-07-2024 04:10:57
*    Last Modified: 14-07-2024 04:10:58
*/


#include "ap_int.h"

#define SIZE 16

void Sbox(unsigned char *in, unsigned char *out);

void SubBytes(unsigned char* state) {
	// #pragma HLS ALLOCATION type=function instances=Sbox limit=4

	for (unsigned char i = 0; i < SIZE; i++) {
		#pragma HLS unroll
		Sbox(&state[i], &state[i]);
	}
}

void AddRoundKey(unsigned char* state, unsigned char* expanded_key) {
	// #pragma HLS inline off

	for (unsigned char i = 0; i < SIZE; i++) {
		#pragma HLS unroll
		state[i] ^= expanded_key[i];
	}
}

void KeyExpansion(unsigned char* key, unsigned char rcon) {
	// #pragma HLS inline off
	unsigned char subbytes_key[4];
	unsigned char expanded_key[SIZE];
	#pragma HLS array_partition variable=expanded_key complete
	#pragma HLS array_partition variable=subbytes_key complete

	// #pragma HLS ALLOCATION type=function instances=Sbox limit=4

	for (unsigned char i = 0; i < 4; i++) {
		#pragma HLS unroll
		Sbox(&key[i + 12], &subbytes_key[i]);
	}

	// rotate word
	unsigned char rot_word[4];
	rot_word[0] = subbytes_key[1] ^ rcon;
	rot_word[1] = subbytes_key[2];
	rot_word[2] = subbytes_key[3];
	rot_word[3] = subbytes_key[0];

	expanded_key[0] = key[0] ^ rot_word[0];
	expanded_key[1] = key[1] ^ rot_word[1];
	expanded_key[2] = key[2] ^ rot_word[2];
	expanded_key[3] = key[3] ^ rot_word[3];
	for (unsigned char i = 4; i < SIZE; i++) {
		#pragma HLS unroll
		expanded_key[i] = expanded_key[i - 4] ^ key[i];
	}

	// update key
	for (unsigned char i = 0; i < SIZE; i++) {
		#pragma HLS unroll
		key[i] = expanded_key[i];
	}
}

ap_int<8> xtime(ap_int<8> x) {
	ap_int<8> y = x(6,0) << 1;
	if (x[7]) {
		y ^= (unsigned char)0x1b;
	}
	return y;
}

void ShiftRows(unsigned char* state) {
	unsigned char next_state[SIZE];
	#pragma HLS array_partition variable=next_state complete

	// 	in state  -> out state
	// 	+---+---+----+----+     +----+----+----+----+
	// 	| 0 | 4 |  8 | 12 |     |  0 |  4 |  8 | 12 |
	// 	+---+---+----+----+     +----+----+----+----+
	// 	| 1 | 5 |  9 | 13 |     |  5 |  9 | 13 |  1 |
	// 	+---+---+----+----+ --> +----+----+----+----+
	// 	| 2 | 6 | 10 | 14 |     | 10 | 13 |  2 |  6 |
	// 	+---+---+----+----+     +----+----+----+----+
	// 	| 3 | 7 | 11 | 15 |     | 15 |  3 |  7 | 11 |
	// 	+---+---+----+----+     +----+----+----+----+

	next_state[0] = state[0];
	next_state[1] = state[5];
	next_state[2] = state[10];
	next_state[3] = state[15];

	next_state[4] = state[4];
	next_state[5] = state[9];
	next_state[6] = state[14];
	next_state[7] = state[3];

	next_state[8] = state[8];
	next_state[9] = state[13];
	next_state[10] = state[2];
	next_state[11] = state[7];

	next_state[12] = state[12];
	next_state[13] = state[1];
	next_state[14] = state[6];
	next_state[15] = state[11];

	for (unsigned char i = 0; i < SIZE; i++) {
		#pragma HLS unroll
		state[i] = next_state[i];
	}
}

void MixSingleColumn(unsigned char* x, unsigned char* y)
{

	//  +----+     +----+----+----+----+   +----+ = +--------------------------------+
	//  | y0 |     | 02 | 03 | 01 | 01 |   | x0 | = | {02}x0 + {02}x1 + x1 + x2 + x3 |
	//  +----+     +----+----+----+----+   +----+ = +--------------------------------+
	//  | y1 |     | 01 | 02 | 03 | 01 |   | x1 | = | x0 + {02}x1 + {02}x2 + x2 + x3 |
	//  +----+  =  +----+----+----+----+ * +----+ = +--------------------------------+
	//  | y2 |     | 01 | 01 | 02 | 03 |   | x2 | = | x0 + x1 + {02}x2 + {02}x3 + x3 |
	//  +----+     +----+----+----+----+   +----+ = +--------------------------------+
	//  | y3 |     | 03 | 01 | 01 | 02 |   | x3 | = | {02}x0 + x0 + x1 + x2 + {02}x3 |
	//  +----+     +----+----+----+----+   +----+ = +--------------------------------+

	unsigned char x_xtime[4];
	for (unsigned char i = 0; i < 4; i++) {
		// #pragma HLS unroll
		#pragma HLS PIPELINE
		x_xtime[i] = xtime(x[i]);
	}

	y[0] = x_xtime[0] ^ x_xtime[1] ^ x[1] ^ x[2] ^ x[3];
	y[1] = x[0] ^ x_xtime[1] ^ x_xtime[2] ^ x[2] ^ x[3];
	y[2] = x[0] ^ x[1] ^ x_xtime[2] ^ x_xtime[3] ^ x[3];
	y[3] = x_xtime[0] ^ x[0] ^ x[1] ^ x[2] ^ x_xtime[3];

}

void MixColumns(unsigned char* state) {
	#pragma HLS inline recursive
	unsigned char next_state[SIZE];
	#pragma HLS array_partition variable=next_state complete

	// #pragma HLS ALLOCATION type=function instances=MixSingleColumn limit=1

	for (unsigned char i = 0; i < 4; i++) {
		#pragma HLS unroll
		MixSingleColumn(&state[i * 4], &next_state[i * 4]);
	}
	for (unsigned char i = 0; i < SIZE; i++) {
		#pragma HLS unroll
		state[i] = next_state[i];
	}
}


void AES128Encrypt(const unsigned char *key, const unsigned char *plaintext, unsigned char *ciphertext)
{
	#pragma HLS INTERFACE mode=m_axi port=key offset=slave bundle=data depth=16
	#pragma HLS INTERFACE mode=m_axi port=plaintext offset=slave bundle=data depth=16
	#pragma HLS INTERFACE mode=m_axi port=ciphertext offset=slave bundle=data depth=16

	// #pragma HLS INTERFACE mode=bram port=key storage_type=ram_1p
	// #pragma HLS INTERFACE mode=bram port=plaintext storage_type=ram_1p
	// #pragma HLS INTERFACE mode=bram port=ciphertext storage_type=ram_1p

	// copy key & plaintext
	unsigned char state[SIZE];
	unsigned char scheduled_key[SIZE];
	ap_int<8> rcon = 1;
	ap_int<8> rcon_shift;

	#pragma HLS array_partition variable=state complete
	#pragma HLS array_partition variable=scheduled_key complete


	LOOP_COPY_IN: for (int i = 0; i < SIZE; i++) {
		#pragma HLS unroll
		state[i] = plaintext[i];
		scheduled_key[i] = key[i];
	}

	AddRoundKey(state, scheduled_key);

	LOOP_ROUND: for (int round = 0; round < 10; round++) {
		#pragma HLS PIPELINE
		SubBytes(state);
		ShiftRows(state);
		if (round != 9) {
			MixColumns(state);
		}
		KeyExpansion(scheduled_key, rcon);
		rcon = xtime(rcon);
		AddRoundKey(state, scheduled_key);
	}

	// output ciphertext
	LOOP_COPY_OUT: for (int i = 0; i < SIZE; i++) {
		#pragma HLS UNROLL
		ciphertext[i] = state[i];
	}

}