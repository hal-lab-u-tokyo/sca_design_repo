#
#    Copyright (C) 2025 The University of Tokyo
#    
#    File:          /aes128_rsm_hls/create_ip.tcl
#    Project:       sca_design_repo
#    Author:        Takuya Kojima in The University of Tokyo (tkojima@hal.ipc.i.u-tokyo.ac.jp)
#    Created Date:  24-01-2025 07:00:59
#    Last Modified: 29-01-2025 03:58:58
#


# default
set target_board ""
set target_freq 50MHz

set script_file [file tail [info script]]
set script_dir [file dirname [info script]]

# option format
# target-board=<board_name> target-freq=<frequency> target-part=<part_name>
if { $::argc > 0 } {
	for {set i 0} {$i < $::argc} {incr i} {
		set option [string trim [lindex $::argv $i]]
		if { [regexp {^target-board=(.+)$} $option -> target_board] } {
			continue
		}
		if { [regexp {^target-freq=(.+)$} $option -> target_freq] } {
			continue
		}
		if { [regexp {^target-part=(.+)$} $option -> target_part] } {
			continue
		}
	}
}

if { $target_board eq "" } {
	puts "ERROR: target-board option is required. Please specify the target board."
	exit
}


open_project -reset hls_${target_board}_aes_rsm_enc

# Add design files
add_files ${script_dir}/src/Sbox_Table.cpp
add_files ${script_dir}/src/Masked_Table.cpp
add_files ${script_dir}/src/tables.h
add_files ${script_dir}/src/RSM_AES_Encrypt.cpp

# Add test bench & files
add_files -tb ${script_dir}/test/test_aes.cpp

# Set the top-level function
set_top RSM_AES128Encrypt

# ########################################################
# Create a solution
open_solution -reset solution0 -flow_target vitis

# Define technology and clock rate
# SAKURA-X Kintex-7
if {${target_board} eq "sakura-x"} {
  set_part {xc7k160tfbg676-1}
} elseif {${target_board} eq "cw305"} {
  set_part {xc7a100tftg256-2}
} else {
  puts "Unknown target board. Please set FPGA part manually."
  set_part ${target_part}
}

# Change the taget clock as you like
create_clock -period "${target_freq}"

# use 32-bit address
config_interface -m_axi_addr64=false

# pre-synthesis C/C++ simulation
csim_design
# run synthesis
csynth_design
#  post-synthesis co-simulation
cosim_design -trace_level all
# export IP for Vivado block design
export_design -format ip_catalog

exit

