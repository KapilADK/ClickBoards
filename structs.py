"""
definiton of c-structs used to communicate with C-Application on RedPitaya
"""

from ctypes import c_char_p, c_int, c_uint32, c_float, Structure, c_double, c_bool


# c-struct for tcp-command
class TcpCommand(Structure):
    _fields_ = [("id", c_int), ("val", c_double), ("ch", c_int)]


# c-structs for sending Config-Params via TCP to RedPitaya
class TriggerConfig(Structure):
    _fields_ = [
        ("trigger_inc", c_int),
        ("trigger_ref_max_value", c_int),
        ("trigger_ref_min_value", c_int),
        ("length", c_int),
        ("start_delay", c_int),
    ]


class DacConfig(Structure):
    _fields_ = [
        ("dev_id", c_int),
        ("mode", c_int),
        ("reset_voltage", c_float),
        ("used_ports", c_int),
        ("mirror", c_int),
    ]


class BramDacConfig(Structure):
    _fields_ = [
        ("port_id", c_int),
        ("dacConfig", DacConfig),
        ("dac_port_id", c_int),
        ("no_steps", c_int),
        ("dwell_time_delay", c_int),
        ("no_sweeps", c_int),
        ("rep_delay", c_int),
        ("enable_handshake", c_int),
    ]


class AdcConfig(Structure):
    _fields_ = [
        ("sample_rate_divider", c_int),
        ("burst_size", c_int),
        ("adc_mode", c_int),
        ("start_delay", c_int),
        ("trigger_mode", c_int),
        ("under_sampling", c_int),
        ("offset_val_port_1", c_int),
        ("offset_val_port_2", c_int),
        ("gain_val_port_1", c_int),
        ("gain_val_port_2", c_int),
        ("debug_port_en", c_int),
        ("gain_offset_calib_en", c_int),
    ]


# Struct to define config for RAM-Writer module
class RamInitConfig(Structure):
    _fields_ = [
        ("sts_width_mask", c_uint32),
        ("tcp_pkg_size", c_uint32),
        ("tcp_pkg_size_bytes", c_uint32),
        ("ram_size", c_uint32),
        ("ram_size_bytes", c_uint32),
    ]


# Struct to define config for dummy data generator
class DummyDataGenConfig(Structure):
    _fields_ = [
        ("mode", c_uint32),
        ("no_samples", c_uint32),
        ("sample_delay", c_uint32),
        ("max_cnt", c_uint32),
    ]


# Struct to define config for Lock-In-Amplifier
class LiaMixerConfig(Structure):
    _fields_ = [
        ("channel", c_uint32),
        ("debug_mode", c_uint32),
    ]


# Struct to define config for LIA's IIR-Filter
class LiaIIRConfig(Structure):
    _fields_ = [
        ("num_sos", c_uint32),
        ("coeffs", c_uint32 * 30),
    ]


# Create Struct to define a LUT-Value
# for adjustment/read/write of single values inside LUT/BRAM
class LutValue(Structure):
    _fields_ = [
        ("port_id", c_int),
        ("dac_dev_id", c_int),
        ("dac_port_id", c_int),
        ("voltage", c_float),
        ("index", c_int),
    ]


# Create struct to define config for clock divider
class ClockDividerConfig(Structure):
    _fields_ = [
        ("divider_param", c_int),
        ("sync_mode_en", c_bool),
    ]


# add more configs for other FPGA-Modules....


# Create struct ti define config for SPI interface
class SpiConfig(Structure):
    _fields_ = [
        ("mode", c_int),
        ("spi_speed", c_int),
    ]


# Create struct which holds 2KB data for sending BRAM data to RP
class BramData(Structure):
    _fields_ = [
        ("size", c_uint32),
        ("data", c_uint32 * 16384),
    ]
