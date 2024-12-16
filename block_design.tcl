# Main-Script for building the block-design...

# EXPLANATION: #############################################################################################

# For bigger scripts you can seperate parts of the design into seperate .tcl-script-files and source them
# with the module-command to create hierarchies inside the block design:

#module HIERARCHY_NAME {
#  source design/*hierarchy-script*.tcl
#} {
#
#  ip_core_inside_hierarchy/portX portY_in_top_blockdesign
#  ...
#  ...
#}

## to simply place one IP-Core use the cell-command:
#
# for xilinx-ips:
#
# cell xilinx.com:ip:xlconstant xlconstant_0 {
#   CONST_WIDTH 1
#   CONST_VAL 1
# } {
#   dout rst_pll_0_125M/ext_reset_in
# }
#
# to place IP with name xlconstant and name it xlconstant_0 inside design
# Connect port dout from xlconstant_0-IP with port ext_reset_int from rst_pll_0_125M-IP
# Also assign values 1 to the Generics of the IP-instance for CONST_WIDTH and CONST_VAL
#
#
# For user IPs use the prefix: hhi-thz-:user:.....
# e.g:
# cell hhi-thz:user:trigger_stretch_v1_0 trigger_stretch_first_sample_out {
#   STRETCH_AMOUNT 1000
# } {
#   clk pll_0/clk_out1
#   trigger_in DAC_INTERFACE/axis_dac_bram_controller_port1/first_sample_out
#   trigger_out first_sample_trigger_o
# }
#
# To map AXI-IPs to the AXI-Address-Bus use the addr-command
#
# addr $AXI_BASE_ADDR_GPIO_LEDS 64k axi_gpio_leds/S_AXI /ps_0/M_AXI_GP0
#
# This maps the axi_gpio_leds-AXI-Slave-Port to AXI_BASE_ADDR_GPIO_LEDS with range AXI_SLAVE_RANGE
# After the mapping the command apply_bd_automation is envoked.. This connects the rest of clock/reset-signals
# for AXI-Slave... This can sometime not work.. So might be a TODO: Optimize bd-automation rule to do it all manually
#
############################################################################################################

#source parameters.tcl from build_src
source ../../../build_src/fpga_tcl/parameters.tcl

# Create clk_wizzard ( to buffer differntial clock from ADC and generate more clocks if needed)
cell xilinx.com:ip:clk_wiz pll_0 {
  PRIMITIVE MMCM
  PRIM_IN_FREQ.VALUE_SRC USER
  SECONDARY_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ $SYS_CLK_FREQ
  SECONDARY_IN_FREQ $SYS_CLK_FREQ
  PRIM_SOURCE Differential_clock_capable_pin
  SECONDARY_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ $SYS_CLK_FREQ
  USE_RESET false
  USE_INCLK_SWITCHOVER true
} {
  clk_in1_p adc_clk_p_i
  clk_in1_n adc_clk_n_i
  clk_in2_p daisy_clock_p_i
  clk_in2_n daisy_clock_n_i
}

# create a constant ip core to select the system clock
cell xilinx.com:ip:xlconstant clock_selecter {
  CONST_WIDTH 1
  CONST_VAL 1
} {
  dout pll_0/clk_in_sel
} 

# Create processing_system7 and apply red_pitaya.xml preset for PS
cell xilinx.com:ip:processing_system7 ps_0 {
  PCW_IMPORT_BOARD_PRESET config/red_pitaya.xml
  PCW_USE_S_AXI_ACP 1
  PCW_USE_DEFAULT_ACP_USER_VAL 1
} {
  M_AXI_GP0_ACLK pll_0/clk_out1
  S_AXI_ACP_ACLK pll_0/clk_out1
}

# Create all required interconnections for PS
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {
  make_external {FIXED_IO, DDR}
  Master Disable
  Slave Disable
} [get_bd_cells ps_0]

#Daisy Chain connectors (rp_cluster)
###############################################################################################################

# route the single-ended clock from clocking wizard to a buffer to get differential clock for FPGA clock source
cell xilinx.com:ip:util_ds_buf utils_ds_buf_2 {
 C_SIZE 1
 C_BUF_TYPE OBUFDS
} {
 OBUF_DS_P adc_enc_p_o
 OBUF_DS_N adc_enc_n_o
 OBUF_IN pll_0/clk_out1
}

# route clock from clocking wizard through a buffer to daisy chain output ports
cell xilinx.com:ip:util_ds_buf utils_ds_buf_3 {
 C_SIZE 1
 C_BUF_TYPE OBUFDS
} {
 OBUF_IN pll_0/clk_out1
 OBUF_DS_P daisy_clock_p_o
 OBUF_DS_N daisy_clock_n_o
}
#################################################################################################################

#AXI GPIO for LEDs
cell xilinx.com:ip:axi_gpio axi_gpio_leds {
  C_GPIO_WIDTH 3
  C_ALL_OUTPUTS 1
} { }

# Map AXI-GPIO to selected base-address (also creates peripheral reset module rst_pll_0_125M)
addr $AXI_BASE_ADDR_GPIO_LEDS $AXI_SLAVE_RANGE axi_gpio_leds/S_AXI /ps_0/M_AXI_GP0

# add axi gpio for rstn-vector (user reset)
cell xilinx.com:ip:axi_gpio axi_gpio_rstn {
  C_GPIO_WIDTH $RESET_WIDTH
  C_ALL_OUTPUTS 1
} { }

#connect axi slave from gpio rstn to axi-bus
addr $AXI_BASE_ADDR_GPIO_RSTN $AXI_SLAVE_RANGE axi_gpio_rstn/S_AXI /ps_0/M_AXI_GP0

#add constant for (width 1, value 1)
# for ext_reset on reset-module (auto generated inside block design with bd-automation)
# for RedPitaya-ADC (first_sample_in), the sync-mode with DAC-BRAM-Controller is not implemented for this template
cell xilinx.com:ip:xlconstant const_1_1 {
  CONST_WIDTH 1
  CONST_VAL 1
} {
  dout rst_pll_0_125M/ext_reset_in
}

module DAC_INTERFACE {
  source design/dac.tcl
} {

  dac_ddr_clk_gen/clk_in1 pll_0/clk_out1

  axis_red_pitaya_dac/rstn axi_gpio_rstn/gpio_io_o
  axis_red_pitaya_dac/dac_clk dac_clk_o
  axis_red_pitaya_dac/dac_rst dac_rst_o
  axis_red_pitaya_dac/dac_wrt dac_wrt_o
  axis_red_pitaya_dac/dac_sel dac_sel_o
  axis_red_pitaya_dac/dac_dat dac_dat_o
  axis_red_pitaya_dac/aclk pll_0/clk_out1
  axis_red_pitaya_dac/s_axi_aresetn rst_pll_0_125M/peripheral_aresetn

  bram_dac_port0/clkb pll_0/clk_out1
  bram_dac_port1/clkb pll_0/clk_out1

  axis_dac_bram_controller_port0/clk pll_0/clk_out1
  axis_dac_bram_controller_port0/rstn axi_gpio_rstn/gpio_io_o
  axis_dac_bram_controller_port0/aclk pll_0/clk_out1
  axis_dac_bram_controller_port0/s_axi_aresetn rst_pll_0_125M/peripheral_aresetn

  axis_dac_bram_controller_port1/clk pll_0/clk_out1
  axis_dac_bram_controller_port1/rstn axi_gpio_rstn/gpio_io_o
  axis_dac_bram_controller_port1/aclk pll_0/clk_out1
  axis_dac_bram_controller_port1/s_axi_aresetn rst_pll_0_125M/peripheral_aresetn

  axi_bram_controller_sync/clk pll_0/clk_out1
  axi_bram_controller_sync/rstn axi_gpio_rstn/gpio_io_o
  axi_bram_controller_sync/s_axi_aresetn rst_pll_0_125M/peripheral_aresetn
  axi_bram_controller_sync/s_axi_aclk pll_0/clk_out1

  util_vector_logic_0/Op2 daisy_dac_trigger_i_ch0
  util_vector_logic_0/Res daisy_dac_trigger_o_ch0

  util_vector_logic_1/Op2 daisy_dac_trigger_i_ch1
  util_vector_logic_1/Res daisy_dac_trigger_o_ch1
}

## add module for stretching first-sample-trigger:
cell hhi-thz:user:trigger_stretch_v1_0 trigger_stretch_first_sample_out {
  STRETCH_AMOUNT 100
} {
  clk pll_0/clk_out1
  trigger_in DAC_INTERFACE/axis_dac_bram_controller_port0/first_sample_out
  trigger_out first_sample_trigger_o
}

module ADC_INTERFACE {
  source design/adc.tcl
} {

  axi_red_pitaya_adc/aclk pll_0/clk_out1
  axi_red_pitaya_adc/s_axi_aresetn rst_pll_0_125M/peripheral_aresetn
  axi_red_pitaya_adc/adc_dat_a adc_dat_a_i
  axi_red_pitaya_adc/adc_dat_b adc_dat_b_i
  axi_red_pitaya_adc/adc_csn adc_csn_o
  axi_red_pitaya_adc/rstn axi_gpio_rstn/gpio_io_o

  axi_red_pitaya_adc/first_sample_in DAC_INTERFACE/axis_dac_bram_controller_port0/first_sample_out
  axi_red_pitaya_adc/burst_trigger DAC_INTERFACE/axis_red_pitaya_dac/sample_trigger_out
}

module RAM_WRITER_INTERFACE {
  source design/ram_writer.tcl
} {

  ram_writer/aclk pll_0/clk_out1
  ram_writer/M_AXI ps_0/S_AXI_ACP
  ram_writer/S_AXIS ADC_INTERFACE/axi_red_pitaya_adc/M_AXIS
  ram_writer/rstn axi_gpio_rstn/gpio_io_o
  ram_writer/s_axi_aresetn rst_pll_0_125M/peripheral_aresetn

}

# add concat to connect sw-leds with hw-leds (for now just constant 0)
cell xilinx.com:ip:xlconcat led_concat {
  NUM_PORTS 3
} {

  In0 DAC_INTERFACE/axis_dac_bram_controller_port0/led_state_out
  In1 DAC_INTERFACE/axis_dac_bram_controller_port0/led_first_sample_out
  In2 axi_gpio_leds/gpio_io_o
  dout led_o
}

# FIX: Error for wrong clock-properties.. maybe there is a smarter way to this....
# SET clock properties for AXI/AXIS interfaces:
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins DAC_INTERFACE/axis_red_pitaya_dac/S00_AXIS]
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins DAC_INTERFACE/axis_red_pitaya_dac/S01_AXIS]
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins DAC_INTERFACE/axis_red_pitaya_dac/S_AXI]
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins DAC_INTERFACE/axis_dac_bram_controller_port0/M_AXIS]
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins DAC_INTERFACE/axis_dac_bram_controller_port1/M_AXIS]
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins DAC_INTERFACE/axis_dac_bram_controller_port0/S_AXI]
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins DAC_INTERFACE/axis_dac_bram_controller_port1/S_AXI]
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins DAC_INTERFACE/axi_bram_controller_sync/S_AXI]
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins ADC_INTERFACE/axi_red_pitaya_adc/M_AXIS]
set_property -dict [list CONFIG.FREQ_HZ {125000000}] [get_bd_intf_pins RAM_WRITER_INTERFACE/ram_writer/S_AXI]

# add axi slaves for RAM-Writer-Interface
addr $AXI_BASE_ADDR_RAM_WRITER $AXI_SLAVE_RANGE RAM_WRITER_INTERFACE/ram_writer/S_AXI /ps_0/M_AXI_GP0

# assign Address for RAM-Connection via AXI_ACP-Port:
assign_bd_address [get_bd_addr_segs ps_0/S_AXI_ACP/ACP_DDR_LOWOCM]

# map axi bram controller instances (one for each port) into axi address space and run autoconnect
addr $AXI_BASE_ADDR_BRAM_PORT0 $BRAM_AXI_RANGE DAC_INTERFACE/axi_bram_ctrl_port0/S_AXI /ps_0/M_AXI_GP0
addr $AXI_BASE_ADDR_BRAM_PORT1 $BRAM_AXI_RANGE DAC_INTERFACE/axi_bram_ctrl_port1/S_AXI /ps_0/M_AXI_GP0

# map axi red pitaya adc:axi_red_pita
addr $AXI_BASE_ADDR_RP_ADC $AXI_SLAVE_RANGE ADC_INTERFACE/axi_red_pitaya_adc/S_AXI /ps_0/M_AXI_GP0

# map axi red pitaya dac:
addr $AXI_BASE_ADDR_RP_DAC $AXI_SLAVE_RANGE DAC_INTERFACE/axis_red_pitaya_dac/S_AXI /ps_0/M_AXI_GP0

# create 2 additionaly ports on axi-smart-connect
set NO_MI_AXI_SMC [get_property CONFIG.NUM_MI [get_bd_cells axi_smc]]
set_property -dict [list CONFIG.NUM_MI [expr $NO_MI_AXI_SMC+3]] [get_bd_cells axi_smc]

# connect bram-port1 and bram-port2 to the last two free master-pots:
connect_bd_intf_net [get_bd_intf_pins axi_smc/M0${NO_MI_AXI_SMC}_AXI] [get_bd_intf_pins DAC_INTERFACE/axis_dac_bram_controller_port0/S_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_smc/M0[expr $NO_MI_AXI_SMC+1]_AXI] [get_bd_intf_pins DAC_INTERFACE/axis_dac_bram_controller_port1/S_AXI]

# assign address in axi-address-space:
assign_bd_address -offset $AXI_BASE_ADDR_DAC_BRAM_CTRL_PORT0 -range $AXI_SLAVE_RANGE [get_bd_addr_segs -of_objects [get_bd_intf_pins DAC_INTERFACE/axis_dac_bram_controller_port0/S_AXI]]
assign_bd_address -offset $AXI_BASE_ADDR_DAC_BRAM_CTRL_PORT1 -range $AXI_SLAVE_RANGE [get_bd_addr_segs -of_objects [get_bd_intf_pins DAC_INTERFACE/axis_dac_bram_controller_port1/S_AXI]]

# map axi interface for BRAM-CTRL-SYNC
addr $AXI_BASE_ADDR_BRAM_CTRL_SYNC $AXI_SLAVE_RANGE DAC_INTERFACE/axi_bram_controller_sync/S_AXI /ps_0/M_AXI_GP0

# add additional register slice inbetwenn PS-Master and Slave-Side on Interconnect to remove timing violations by adding pipeline-stages
#1. remove connection to M-GPO on Zynq-PS
delete_bd_objs [get_bd_intf_nets ps_0_M_AXI_GP0]
#2. place register slice
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_0
endgroup
#3. connect ports
connect_bd_intf_net [get_bd_intf_pins axi_register_slice_0/S_AXI] [get_bd_intf_pins ps_0/M_AXI_GP0]
connect_bd_intf_net [get_bd_intf_pins axi_register_slice_0/M_AXI] [get_bd_intf_pins axi_smc/S00_AXI]
connect_bd_net [get_bd_pins axi_register_slice_0/aclk] [get_bd_pins pll_0/clk_out1]
connect_bd_net [get_bd_pins axi_register_slice_0/aresetn] [get_bd_pins rst_pll_0_125M/peripheral_aresetn]
## FIX: This option fixed the latest timining violations regarding the Axi-SmartConnect:
#4. now eneable Auto-Pipelining by setting all Registers to "Multi SLR Crossing" & enable "timing-driven pipeline insertion for all Multi-SLR"
startgroup
set_property -dict [list CONFIG.REG_AW {15} CONFIG.REG_AR {15} CONFIG.REG_W {15} CONFIG.REG_R {15} CONFIG.REG_B {15} CONFIG.USE_AUTOPIPELINING {1}] [get_bd_cells axi_register_slice_0]
endgroup
