#source parameters.tcl from build_src
source ../../../build_src/fpga_tcl/parameters.tcl

cell hhi-thz:user:axis_ram_writer_v2_0 ram_writer {
    RESET_WIDTH $RESET_WIDTH
    RESET_INDEX $RESET_INDEX_RAM_WRITER
    STS_DATA_WIDTH 32
    CFG_DATA_WIDTH 32
    M_AXI_ID_WIDTH 3
    M_AXI_ADDR_WIDTH 32
    M_AXI_DATA_WIDTH 64
    AXI_ADDR_WIDTH 6
    AXI_DATA_WIDTH 32
    S_AXIS_TDATA_WIDTH 32
    FIFO_WRITE_DEPTH 1024
} {
}
