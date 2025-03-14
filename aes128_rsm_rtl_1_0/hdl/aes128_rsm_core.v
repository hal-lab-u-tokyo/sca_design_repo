//
//    Copyright (C) 2025 The University of Tokyo
//    
//    File:          /aes128_rsm_rtl_1_0/hdl/aes128_rsm_core.v
//    Project:       sca_design_repo
//    Author:        Takuya Kojima in The University of Tokyo (tkojima@hal.ipc.i.u-tokyo.ac.jp)
//    Created Date:  14-03-2025 04:35:17
//    Last Modified: 14-03-2025 05:40:25
//



module AES128_RSM_Core
(
	input clk,
	input reset_n,
	input [3:0] i_rotate,
	input [127:0] i_key,
	input i_key_valid,
	input [127:0] i_plaintext,
	input i_plaintext_valid,
	output [127:0] o_ciphertext,
	output o_ciphertext_valid,
	output o_busy
);

	reg [127:0] r_state;
	reg [127:0] r_round_key;
	wire [127:0] w_next_key;
	wire [127:0] w_next_state;
	reg [9:0] r_round;
	reg [3:0] r_rotate;
	reg [7:0] rcon;
	reg r_ciphertext_valid;

	KeyExpantion KE0 (
		.kin(r_round_key), .kout(w_next_key), .rcon(rcon)
	);

	AES_Round AR0 (
		.i_state(r_state), .i_key(r_round_key), .i_rotate(r_rotate), .i_last_round(r_round[9]), .o_state(w_next_state)
	);

	assign o_ciphertext = r_state;
	assign o_ciphertext_valid = r_ciphertext_valid;
	assign o_busy = |r_round || i_plaintext_valid || r_ciphertext_valid;

`define USE_M_TABLE
`include "rsm_tables.v"
`undef USE_M_TABLE

	wire [127:0] M = M_TABLE(i_rotate);
	always @(posedge clk or negedge reset_n) begin
		if (~reset_n) begin
			r_state <= 0;
		end else if (i_plaintext_valid) begin
			r_state <= i_plaintext ^ M_TABLE(i_rotate) ^ i_key;
		end else if (|r_round) begin
			r_state <= w_next_state;
		end
	end

	always @(posedge clk or negedge reset_n) begin
		if (~reset_n) begin
			r_round <= 10'd0;
		end else if (i_plaintext_valid & !(|r_round)) begin
			r_round <= 10'd1;
		end else if (|r_round) begin
			r_round <= {r_round[8:0], 1'b0};
		end
	end

	always @(posedge clk or negedge reset_n) begin
		if (~reset_n) begin
			r_rotate <= 0;
		end else if (|r_round) begin
			r_rotate <= r_rotate + 1;
		end else begin
			r_rotate <= i_rotate;
		end
	end

	always @(posedge clk or negedge reset_n) begin
		if (~reset_n) begin
			r_round_key <= 0;
		end else if (i_key_valid) begin
			r_round_key <= i_key;
		end else if(|r_round || i_plaintext_valid) begin
			r_round_key <= w_next_key;
		end
	end

	always @(posedge clk or negedge reset_n) begin
		if (~reset_n) begin
			rcon <= 1;
		end else if (|r_round || i_plaintext_valid) begin
			rcon <= (rcon[7]) ? {rcon[6:0], 1'b0} ^ 8'h1B : {rcon[6:0], 1'b0};
		end else begin
			rcon <= 1;
		end
	end

	always @(posedge clk or negedge reset_n) begin
		if (~reset_n) begin
			r_ciphertext_valid <= 0;
		end else begin
			r_ciphertext_valid <= r_round[9];
		end
	end

endmodule

module AES_Round(
	input [127:0] i_state,
	input [127:0] i_key,
	input [3:0] i_rotate,
	input i_last_round,
	output [127:0] o_state
);

	wire [31:0] sb0, sb1, sb2, sb3, // SubBytes
				sr0, sr1, sr2, sr3, // ShiftRows
				sc0, sc1, sc2, sc3; // MixColumns

	// SubBytes
	MaskedSubBytes SB0 (i_rotate, i_state, {sb3, sb2, sb1, sb0});

	// ShiftRows
	assign sr0 = {sb0[31:24], sb3[23:16], sb2[15:8], sb1[7:0]};
	assign sr1 = {sb1[31:24], sb0[23:16], sb3[15:8], sb2[7:0]};
	assign sr2 = {sb2[31:24], sb1[23:16], sb0[15:8], sb3[7:0]};
	assign sr3 = {sb3[31:24], sb2[23:16], sb1[15:8], sb0[7:0]};

	// MixColumns
	MixColumns MC0 (sr0, sc0);
	MixColumns MC1 (sr1, sc1);
	MixColumns MC2 (sr2, sc2);
	MixColumns MC3 (sr3, sc3);

	// Mask tables
	`define USE_MMS_TABLE
	`define USE_MS_TABLE
	`include "rsm_tables.v"
	`undef USE_MMS_TABLE
	`undef USE_MS_TABLE

	wire [127:0] MMS;
	wire [127:0] MS;
	assign MMS = MMS_TABLE(i_rotate + 1);
	assign MS = MS_TABLE(i_rotate + 1);

	// AddRoundKey with mask
	assign o_state = (i_last_round) ? {sr3, sr2, sr1, sr0} ^ MS ^ i_key :
		{sc3, sc2, sc1, sc0} ^ MMS ^ i_key;

endmodule

module MaskedSubBytes
(
	input [3:0] i_rotate,
	input [127:0] i_state,
	output [127:0] o_state
);

	`define USE_MASKED_SBOX
	`include "rsm_tables.v"
	`undef USE_MASKED_SBOX

	wire [127:0] w_i_state_shifted;
	wire [7:0] sbox_in0, sbox_in1, sbox_in2, sbox_in3;
	wire [7:0] sbox_in4, sbox_in5, sbox_in6, sbox_in7;
	wire [7:0] sbox_in8, sbox_in9, sbox_in10, sbox_in11;
	wire [7:0] sbox_in12, sbox_in13, sbox_in14, sbox_in15;

	wire [127:0] w_o_state_shifted;
	wire [7:0] sbox_out0, sbox_out1, sbox_out2, sbox_out3;
	wire [7:0] sbox_out4, sbox_out5, sbox_out6, sbox_out7;
	wire [7:0] sbox_out8, sbox_out9, sbox_out10, sbox_out11;
	wire [7:0] sbox_out12, sbox_out13, sbox_out14, sbox_out15;

	// Barrel shift
	assign w_i_state_shifted = (i_state >> (i_rotate * 8)) | (i_state << (128 - (i_rotate * 8)));

	assign sbox_in0 = w_i_state_shifted[127:120];
	assign sbox_in1 = w_i_state_shifted[119:112];
	assign sbox_in2 = w_i_state_shifted[111:104];
	assign sbox_in3 = w_i_state_shifted[103:96];
	assign sbox_in4 = w_i_state_shifted[95:88];
	assign sbox_in5 = w_i_state_shifted[87:80];
	assign sbox_in6 = w_i_state_shifted[79:72];
	assign sbox_in7 = w_i_state_shifted[71:64];
	assign sbox_in8 = w_i_state_shifted[63:56];
	assign sbox_in9 = w_i_state_shifted[55:48];
	assign sbox_in10 = w_i_state_shifted[47:40];
	assign sbox_in11 = w_i_state_shifted[39:32];
	assign sbox_in12 = w_i_state_shifted[31:24];
	assign sbox_in13 = w_i_state_shifted[23:16];
	assign sbox_in14 = w_i_state_shifted[15:8];
	assign sbox_in15 = w_i_state_shifted[7:0];

	// Masked sbox lookup
	assign sbox_out0 = sbox_table_0(sbox_in0);
	assign sbox_out1 = sbox_table_1(sbox_in1);
	assign sbox_out2 = sbox_table_2(sbox_in2);
	assign sbox_out3 = sbox_table_3(sbox_in3);
	assign sbox_out4 = sbox_table_4(sbox_in4);
	assign sbox_out5 = sbox_table_5(sbox_in5);
	assign sbox_out6 = sbox_table_6(sbox_in6);
	assign sbox_out7 = sbox_table_7(sbox_in7);
	assign sbox_out8 = sbox_table_8(sbox_in8);
	assign sbox_out9 = sbox_table_9(sbox_in9);
	assign sbox_out10 = sbox_table_10(sbox_in10);
	assign sbox_out11 = sbox_table_11(sbox_in11);
	assign sbox_out12 = sbox_table_12(sbox_in12);
	assign sbox_out13 = sbox_table_13(sbox_in13);
	assign sbox_out14 = sbox_table_14(sbox_in14);
	assign sbox_out15 = sbox_table_15(sbox_in15);

	assign w_o_state_shifted = {sbox_out0, sbox_out1, sbox_out2, sbox_out3, sbox_out4, sbox_out5, sbox_out6, sbox_out7, sbox_out8, sbox_out9, sbox_out10, sbox_out11, sbox_out12, sbox_out13, sbox_out14, sbox_out15};

	// Barrel shift
	assign o_state = (w_o_state_shifted << (i_rotate * 8)) | (w_o_state_shifted >> (128 - (i_rotate * 8)));



endmodule