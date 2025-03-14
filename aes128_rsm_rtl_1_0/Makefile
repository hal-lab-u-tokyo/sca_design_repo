#
#    Copyright (C) 2025 The University of Tokyo
#    
#    File:          /aes128_rsm_rtl_1_0/Makefile
#    Project:       sca_design_repo
#    Author:        Takuya Kojima in The University of Tokyo (tkojima@hal.ipc.i.u-tokyo.ac.jp)
#    Created Date:  29-01-2025 04:44:37
#    Last Modified: 13-03-2025 08:17:27
#

all: hdl/rsm_tables.v

SEED ?= 10

hdl/rsm_tables.v:
	python generate_mask.py -o $@ --seed ${SEED}

.PHONY: clean
clean:
	rm -f hdl/rsm_tables.v