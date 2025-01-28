###
#   Copyright (C) 2025 The University of Tokyo
#   
#   File:          /aes128_rsm_hls/generate_mask.py
#   Project:       sca_design_repo
#   Author:        Takuya Kojima in The University of Tokyo (tkojima@hal.ipc.i.u-tokyo.ac.jp)
#   Created Date:  29-01-2025 04:44:52
#   Last Modified: 29-01-2025 04:45:01
###

from chipwhisperer.analyzer.utils.aes_funcs import *
import numpy as np
import argparse

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

def numpy_to_c_source_str(array, type_name, var_name):
    contents = ""
    def format_array(array, indent=0):
        if array.ndim == 1:
            return "{" + ", ".join(map(str, array)) + "}"
        else:
            indent_space = " " * indent
            inner = ",\n".join(
                indent_space + format_array(subarray, indent + 4) for subarray in array
            )
        return "{\n" + inner + "\n" + " " * (indent - 4) + "}"

    contents += f"{type_name} {var_name}"
    contents += "[" + "][".join(map(str, array.shape)) + "] = "
    contents += format_array(array, indent=4)
    contents += ";\n\n"

    return contents

def save_mask_to_c(sboxes, M, MMS, MS, filename):
    contents = "#include \"tables.h\"\n\n"
    contents += numpy_to_c_source_str(sboxes, "const unsigned char", "masked_sboxes")
    contents += numpy_to_c_source_str(M, "const unsigned char", "M")
    contents += numpy_to_c_source_str(MMS, "const unsigned char", "MMS")
    contents += numpy_to_c_source_str(MS, "const unsigned char", "MS")

    print(filename)
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
    save_mask_to_c(sboxes, M, MMS, MS, args.output)