#
#    Copyright (C) 2025 The University of Tokyo
#    
#    File:          /aes128_rsm_hls/Makefile
#    Project:       sca_design_repo
#    Author:        Takuya Kojima in The University of Tokyo (tkojima@hal.ipc.i.u-tokyo.ac.jp)
#    Created Date:  29-01-2025 04:44:37
#    Last Modified: 29-01-2025 04:44:38
#


TARGET_FREQ ?= 10
TARGET_BOARD ?= sakura-x

all: hls_${TARGET_BOARD}_aes_rsm_enc/kernel.xml

src/Masked_Table.cpp:
	python generate_mask.py -o $@

hls_${TARGET_BOARD}_aes_rsm_enc/kernel.xml: src/Masked_Table.cpp
	vitis_hls -f create_ip.tcl target-board=${TARGET_BOARD} target-freq=${TARGET_FREQ}MHz
