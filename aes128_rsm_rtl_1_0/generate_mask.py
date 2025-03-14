###
#   Copyright (C) 2025 The University of Tokyo
#   
#   File:          /aes128_rsm_rtl_1_0/generate_mask.py
#   Project:       sca_design_repo
#   Author:        Takuya Kojima in The University of Tokyo (tkojima@hal.ipc.i.u-tokyo.ac.jp)
#   Created Date:  29-01-2025 04:44:52
#   Last Modified: 13-03-2025 10:36:06
###

from chipwhisperer.analyzer.utils.aes_funcs import *
import numpy as np
import argparse

sbox_table_fmt = """
function [7:0] {name:s};
    input [7:0] x;
    reg [7:0] t;
    begin
        case (x)
            {table_contents:s}
        endcase
        {name} = t;
    end
endfunction
"""

sbox_table_entry_fmt = "8'h{0:02x}: t = 8'h{1:02x};"

mask_table_fmt = """
function [127:0] {name:s};
    input [3:0] x;
    reg [127:0] t;
    begin
        case (x)
{table_contents:s}
        endcase
        {name} = t;
    end
endfunction
"""

mask_table_entry_fmt = "            4'h{0:x}: t = 128'h{1:s};"

def generate_mask():
    # 16 bytes random number
    mask = np.random.randint(0, 256, 16, dtype=np.uint8)
    sboxes = []
    for i in range(16):
        sbox_table = np.array([sbox(x ^ mask[i]) ^ mask[(i+1)%16] for x in range(256)], dtype=np.uint8)
        sboxes.append(sbox_table)
    sboxes = np.array(sboxes, dtype=np.uint8)
    M = [np.concatenate((mask[i:], mask[:i])) for i in range(16)]
    M = np.array(M, dtype=np.uint8)

    MMS = [np.array(mixcolumns(shiftrows(bytearray(M[i]))), dtype=np.uint8) ^ M[i] for i in range(16)]
    MMS = np.array(MMS, dtype=np.uint8)

    MS = [shiftrows(bytearray(M[i]))  for i in range(16)]
    MS = np.array(MS, dtype=np.uint8)

    return sboxes, M, MMS, MS

def sbox_to_verilog(sboxes):
    contents = "`ifdef USE_MASKED_SBOX\n"
    for i, sbox in enumerate(sboxes):
        table_name = f"sbox_table_{i}"
        table_contents = " ".join([sbox_table_entry_fmt.format(x, sbox[x]) for x in range(256)])
        contents += sbox_table_fmt.format(name=table_name, table_contents=table_contents)
    contents += "`endif\n"
    return contents

def mask_to_verilog(mask_table, name):
    contents = f"`ifdef USE_{name}\n"
    table_contents = ""
    for i, mask in enumerate(mask_table):
        table_contents += mask_table_entry_fmt.format(i, "_".join([f"{x:02x}" for x in mask[::]])) + "\n"
    contents += mask_table_fmt.format(name=name, table_contents=table_contents)
    contents += "`endif\n"

    return contents

def save_mask_to_verilog(sboxes, M, MMS, MS, filename):

    contents = sbox_to_verilog(sboxes)
    contents += mask_to_verilog(M, "M_TABLE")
    contents += mask_to_verilog(MMS, "MMS_TABLE")
    contents += mask_to_verilog(MS, "MS_TABLE")

    with open(filename, "w") as f:
        f.write(contents)

def arg_parse():
    parser = argparse.ArgumentParser(description="Generate RSM constant tables")
    parser.add_argument("-o", "--output", type=str, default="Masked_Table.cpp", help="Output file name")
    parser.add_argument("-s", "--seed", type=int, help="Random seed")
    return parser.parse_args()

if __name__ == "__main__":
    args = arg_parse()
    if args.seed is not None:
        np.random.seed(args.seed)

    sboxes, M, MMS, MS = generate_mask()
    save_mask_to_verilog(sboxes, M, MMS, MS, args.output)