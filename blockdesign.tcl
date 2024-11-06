startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:util_ds_buf:2.1 util_ds_buf_0
endgroup
set_property -dict [list CONFIG.C_BUF_TYPE {OBUFDS}] [get_bd_cells util_ds_buf_0]
set_property location {7 2149 -31} [get_bd_cells util_ds_buf_0]
connect_bd_net [get_bd_pins ps_0/FCLK_CLK0] [get_bd_pins util_ds_buf_0/OBUF_IN]
connect_bd_net [get_bd_ports adc_enc_p_o] [get_bd_pins util_ds_buf_0/OBUF_DS_P]
connect_bd_net [get_bd_ports adc_enc_n_o] [get_bd_pins util_ds_buf_0/OBUF_DS_N]
startgroup
set_property -dict [list CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {125}] [get_bd_cells ps_0]
endgroup
