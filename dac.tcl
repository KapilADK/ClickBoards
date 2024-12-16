## This Module contains the interface for the fast DAC (axi stream - BRAM)
## and the slow DACs (PDM- axi lite interface via PS)

#source parameters.tcl from build_src
source ../../../build_src/fpga_tcl/parameters.tcl

# clk wizzard for fast clock RP-DAC (wrt-clock)
cell xilinx.com:ip:clk_wiz dac_ddr_clk_gen {
  CLKOUT2_USED false
  CLKOUT1_REQUESTED_OUT_FREQ $RP_DAC_CLK_FREQ
  USE_RESET false
  MMCM_CLKOUT0_DIVIDE_F 4.000
  MMCM_CLKOUT1_DIVIDE 1
  NUM_OUT_CLKS 1
  CLKOUT1_JITTER 110.209
  CLKOUT2_JITTER 130.958
  CLKOUT2_PHASE_ERROR 98.575
} {

}

# for DAC-Port 0
cell xilinx.com:ip:blk_mem_gen:8.4 bram_dac_port0 {
  Memory_type True_Dual_Port_RAM
  Enable_32bit_Address true
  Use_Byte_Write_Enable true
  Byte_Size 8
  Assume_Synchronous_Clk false
  Enable_B Always_Enabled
  Register_PortA_Output_of_Memory_Primitives false
  Register_PortB_Output_of_Memory_Primitives false
  Use_RSTA_Pin true
  Use_RSTB_Pin true
  Port_A_Write_Rate 50
  Port_B_Clock 125
  Port_B_Write_Rate 50
  Port_B_Enable_Rate 100
  use_bram_block BRAM_Controller
  EN_SAFETY_CKT true
  } {
  }

# add axis_dac_bram_controller for port 0
cell hhi-thz:user:axis_dac_bram_controller_v3_0 axis_dac_bram_controller_port0 {
  RESET_WIDTH $RESET_WIDTH
  RESET_INDEX $RESET_INDEX_DAC_BRAM_CTRL_PORT0
  CHANNEL_SEL 0
  EN_LDAC_SYNC 1
  } {
    bram_we bram_dac_port0/web
    bram_addr bram_dac_port0/addrb
    data_out_to_bram  bram_dac_port0/dinb
    bram_reset bram_dac_port0/rstb
    data_in_from_bram bram_dac_port0/doutb
  }

#port 0 AXI-Bram-Controller for PS-Connection to LUT
cell xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_port0 {
  PROTOCOL {AXI4}
  SINGLE_PORT_BRAM {1}
} {
  BRAM_PORTA bram_dac_port0/BRAM_PORTA
}

# for DAC-Port 1
cell xilinx.com:ip:blk_mem_gen:8.4 bram_dac_port1 {
  Memory_type True_Dual_Port_RAM
  Enable_32bit_Address true
  Use_Byte_Write_Enable true
  Byte_Size 8
  Assume_Synchronous_Clk false
  Enable_B Always_Enabled
  Register_PortA_Output_of_Memory_Primitives false
  Register_PortB_Output_of_Memory_Primitives false
  Use_RSTA_Pin true
  Use_RSTB_Pin true
  Port_A_Write_Rate 50
  Port_B_Clock 125
  Port_B_Write_Rate 50
  Port_B_Enable_Rate 100
  use_bram_block BRAM_Controller
  EN_SAFETY_CKT true
  } {
  }

# add axis_dac_bram_controller for port 1
cell hhi-thz:user:axis_dac_bram_controller_v3_0 axis_dac_bram_controller_port1 {
  RESET_WIDTH $RESET_WIDTH
  RESET_INDEX $RESET_INDEX_DAC_BRAM_CTRL_PORT1
  CHANNEL_SEL 0
  EN_LDAC_SYNC 0
  } {
    bram_we bram_dac_port1/web
    bram_addr bram_dac_port1/addrb
    data_out_to_bram  bram_dac_port1/dinb
    bram_reset bram_dac_port1/rstb
    data_in_from_bram bram_dac_port1/doutb
  }

#port 1 AXI-Bram-Controller for PS-Connection to LUT
cell xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_port1 {
  PROTOCOL {AXI4}
  SINGLE_PORT_BRAM {1}
} {
  BRAM_PORTA bram_dac_port1/BRAM_PORTA
}

cell hhi-thz:user:axi_bram_controller_sync_v1_0 axi_bram_controller_sync {
  RESET_WIDTH $RESET_WIDTH
  RESET_INDEX $RESET_INDEX_BRAM_CTRL_SYNC
} {
}


#add red pitaya DAC axis interface IP:
cell hhi-thz:user:axis_red_pitaya_dac_v4_0 axis_red_pitaya_dac {
  RESET_WIDTH $RESET_WIDTH
  RESET_INDEX $RESET_INDEX_RP_DAC
  AXIS_TDATA_WIDTH 16
  AXIS_TUSER_WIDTH 32
} {
  ddr_clk dac_ddr_clk_gen/clk_out1
  wrt_clk dac_ddr_clk_gen/clk_out1
  locked dac_ddr_clk_gen/locked
  S00_AXIS axis_dac_bram_controller_port0/M_AXIS
  S01_AXIS axis_dac_bram_controller_port1/M_AXIS
}

# add utility vector logic to for starting output on ch0
cell xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_0 {
  C_SIZE 1
  C_OPERATION or
} {
  Op1 axi_bram_controller_sync/start_bram_output_ch0
  Res axis_dac_bram_controller_port0/start_bram_output
}

#add utility vector logic to for starting output on ch1
cell xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_1 {
  C_SIZE 1
  C_OPERATION or
} {
  Op1 axi_bram_controller_sync/start_bram_output_ch1
  Res axis_dac_bram_controller_port1/start_bram_output
}
