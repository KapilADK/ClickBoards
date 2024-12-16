# This moduel contains the interface for controlling the 4 slow ADC-ports
# on RedPitaya. With this they are accesible via AXI-lite from
# PS by polling regs 0x00 to 0x0C

# Added fast rf dac to interface:

#source parameters.tcl from build_src
source ../../../build_src/fpga_tcl/parameters.tcl

#cell xilinx.com:ip:xadc_wiz xadc {
#    INTERFACE_SELECTION None
#    XADC_STARUP_SELECTION channel_sequencer
#    ENABLE_AXI4STREAM true
#    DCLK_FREQUENCY 125
#    CHANNEL_ENABLE_VP_VN false
#    CHANNEL_ENABLE_VAUXP0_VAUXN0 true
#    CHANNEL_ENABLE_VAUXP1_VAUXN1 true
#    CHANNEL_ENABLE_VAUXP8_VAUXN8 true
#    CHANNEL_ENABLE_VAUXP9_VAUXN9 true
#    ENABLE_RESET false
#    WAVEFORM_TYPE CONSTANT
#    SEQUENCER_MODE Continuous
#    ADC_CONVERSION_RATE 1000
#    EXTERNAL_MUX_CHANNEL VP_V
#    SINGLE_CHANNEL_SELECTION TEMPERATURE
#    STIMULUS_FREQ 1.0
#}
#
#cell hhi-thz:user:axi_xadc_controller_v1_0 axi_xadc_controller {
#
#    RESET_WIDTH $RESET_WIDTH
#    RESET_INDEX $RESET_INDEX_XADC_CTRL
#
#} {
#
#    s_axis xadc/M_AXIS
#}

cell hhi-thz:user:axi_red_pitaya_adc_v1_0 axi_red_pitaya_adc {
    RESET_WIDTH $RESET_WIDTH
    RESET_INDEX $RESET_INDEX_RP_ADC
    TRIGGER_FROM_SAME_CLOCK 1

} {

}
