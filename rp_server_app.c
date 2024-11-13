/*
 * rp_server_app.c
 *
 *  Created on: 27.11.2022
 *      Author: walter
 */
 #include <stdlib.h>
#include <signal.h>  //sig_atomic_t
#include <stdio.h>   //printf
#include <stdnoreturn.h>
#include <sys/socket.h>  //listen
#include <sys/types.h>
#include "rp_server_app.h"
#include "dac/dac_constants.h"
#include "rp_adc.h"
#include "rp_constants.h"
#include "rp_dac.h"
#include "rp_dac_bram_ctrl.h"
#include "rp_lut.h"
#include "rp_misc.h"
#include "rp_parameters.h"
#include "rp_pwm.h"
#include "rp_ram.h"
#include "rp_reset.h"
#include "rp_tcp.h"
#include "rp_trigger_gen.h"
#include "rp_dummy_data_gen.h"
#include "rp_clock_divider.h"
#include "rp_lia.h"
#include "rp_axis_mux.h"
#include "rp_click_boards/adc24click.h"
#include "rp_click_boards/adc20click.h"
#include "rp_spi.h"

volatile sig_atomic_t interrupted = 0;  // global flag to track if application got interrupted by user

void signal_handler(int sig) {
    // handles user interrupt (ctrl+c), gets connected to
    printf("interrupted!\n");
    interrupted = 1;
}

int app_server(AxiDevs axi_devs, bool verbose) {
    // after all devices are initialized this should be the main application loop,
    // where all requests from the application connected via tcp are handled

    int sock_client;  // client which is connected to server
    int sock_server;
    uint16_t cmd;
    RamConfig ramCfg;

    bool initial_start = true;  // flag to show the inital start

    // set to false at some point....

    // init buffer for TCP-Command (struct)
    TcpCmd command;
    char TcpCmdBuffer[sizeof(TcpCmd)];

    // Initalize a config-struct for each module where we want to receive configs for:
    DacConfig dacCfg;
    char DacConfigBuffer[sizeof(DacConfig)];
    AdcConfig adcCfg;
    char AdcConfigBuffer[sizeof(AdcConfig)];
    BramDacConfig bramDacConfig;
    BramDacConfig bramDacConfig_arr[NO_DAC_BRAM_INTERFACES_USED];
    char BramDacConfigBuffer[sizeof(BramDacConfig)];
    TriggerConfig triggerConfig;
    char TriggerConfigBuffer[sizeof(TriggerConfig)];
    DummyDataGenConfig dummyCfg;
    char dummyCfgBuffer[sizeof(DummyDataGenConfig)];
    LiaMixerConfig liaMixerCfg;
    char liaMixerCfgBuffer[sizeof(LiaMixerConfig)];
    ClockDividerConfig clockDividerConfig;
    char ClockDividerConfigBuffer[sizeof(ClockDividerConfig)];
    RamInitConfig ramInitCfg;
    char ramInitCfgBuffer[sizeof(RamInitConfig)];
    BramConfig bramCfg;
    char BramCfgBuffer[sizeof(BramConfig)];
    LiaIIRConfig liaIIRCfg;
    char liaIIRCfgBuffer[sizeof(LiaIIRConfig)];
    SpiConfig spiCfg;
    char spiCfgBuffer[sizeof(SpiConfig)];

    // store file descriptor of the SPI interface
    int spi_fd;

    int size; int *ptr;

    // store no of tcp packages for ADC-TCP-Connection
    int no_tcp_packages;

    // for storing read-out-voltage from get-functions...
    int voltage_mV, adc_cnt;

    // when setting voltage on DAC
    float voltage_V;

    // for trigger-dac-sweep:
    int trigger_sweep_index;
    int no_total_trigger;
    LutValue lutValue;
    char LutValueBuffer[sizeof(LutValue)];

    // for DAC-BRAM Info
    float sample_rate_kHz, signal_rate_kHz;
    int sample_rate_cnt, signal_rate_cnt;

    // intit Server
    sock_server = init_server();

    // init system...
    disable_system(axi_devs);  // disable all FPGA-Modules on default, activated only after config-params got send?

    printf("App-Server started..\n");

    listen(sock_server, 1024);

    // bind user interrupt (^C => SIGINT) to "interrupt handler"
    signal(SIGINT, signal_handler);

    // main while loop:
    while (!interrupted) {
        printf("Waiting for client to connect to Socket..\n");

        sock_client = wait_for_client_connect(sock_server);

        printf("Client connected to Socket...\n");

        while (!interrupted) {
            printf("... waiting for next command\n");
            wait_for_new_command(sock_client,&command,TcpCmdBuffer,sizeof(TcpCmdBuffer));
            if (verbose) printf(">> msg_received: ID: %d , channel: %d ,value: %f \n", command.id, command.ch, command.val);

            switch (command.id) {
                case NEW_CONFIG:
                    switch ((int)command.val) {
                        case DAC_CONFIG_ID:
                            // Receive new DAC configuration
                            receive_struct(sock_client, &dacCfg, DacConfigBuffer, sizeof(DacConfig));
                            printf("\n### Received new DAC-Config ###\n");
                            // Configure and initialize output to init-state
                            init_dac_module(axi_devs, dacCfg, false);  // Do not reset DAC-Outputs afer conifg (false)
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            break;

                        case ADC_CONFIG_ID:
                            // Receive new ADC configuration
                            receive_struct(sock_client, &adcCfg, AdcConfigBuffer, sizeof(AdcConfig));
                            printf("\n### Received new ADC-Config ###\n");
                            // Configure ADC
                            rpa_config(axi_devs, adcCfg, verbose);
                            // Configure RAM to enable/disable Block-Mode
                            set_ram_writer_mode(axi_devs, adcCfg.adc_mode);
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            break;

                        case DAC_BRAM_CONFIG_ID:
                            // Receive new BRAM-DAC configuration
                            receive_struct(sock_client, &bramDacConfig, BramDacConfigBuffer, sizeof(BramDacConfig));
                            printf("\n### Received new BRAM-DAC-config ###\n");
                            // store new bramDacConfig at index port_id in bramDacConfig-array:
                            // so we have acces to each config for each port only by knowing the port-id
                            bramDacConfig_arr[bramDacConfig.port_id] = bramDacConfig;
                            // TODOO: Add check if selected dac_bram_controller is running and skip the config???
                            // ... not really needed when we dont disable DAC-Outputs after Config
                            // Configure DacBramController for given port in bramDacConfig
                            configDacBramController(axi_devs, bramDacConfig, verbose);
                            // Write DAC-LUT-Values for selected BRAM-DAC-Port X (from file in lut/lut_port0.csv)
                            write_dac_lut_from_config(axi_devs, bramDacConfig, verbose);
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            break;

                        case LUT_CONFIG_ID:
                            // Handle LUT configuration (if needed)
                            break;

                        case TRIGGER_CONFIG_ID:
                            // receive new trigger-config:
                            receive_struct(sock_client, &triggerConfig, TriggerConfigBuffer, sizeof(TriggerConfig));
                            printf("\n### Received new Trigger-config ###\n");
                            // config trigger generator with calculated values from host:
                            config_trigger_generator(axi_devs, triggerConfig);
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            break;

                        case DUMMY_DATA_GEN_CONFIG_ID:
                            receive_struct(sock_client, &dummyCfg, dummyCfgBuffer, sizeof(DummyDataGenConfig));
                            printf("\n### Received new Dummy-Data-Generator-Config ###\n");
                            // config Dummy Data Generator with values from host:
                            config_dummy_data_gen(axi_devs, dummyCfg, verbose);
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            break;

                        case DUMMY_DATA_GEN_BRAM_ID:
                            // Receive new data for Dummy-Data-Generator
                            receive_struct(sock_client, &bramCfg, BramCfgBuffer, sizeof(BramConfig));
                            printf("\n### Received new BRAM data ###\n");
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            write_dummy_data_gen_bram(axi_devs, bramCfg, verbose);
                            break;

                        case LIA_MIXER_CONFIG_ID:
                            // Receive new LIA-Mixer-Config
                            receive_struct(sock_client, &liaMixerCfg, liaMixerCfgBuffer, sizeof(LiaMixerConfig));
                            printf("\n### Received new LIA-Mixer-Config ###\n");
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            config_lia_mixer(axi_devs, liaMixerCfg, verbose);
                            break;

                        case LIA_MIXER_BRAM_ID:
                            // Receive new data for LIA-Mixer
                            receive_struct(sock_client, &bramCfg, BramCfgBuffer, sizeof(BramConfig));
                            printf("\n### Received new BRAM data ###\n");
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            write_lia_mixer_bram(axi_devs, bramCfg, verbose);
                            break;

                        case LIA_IIR_CONFIG_ID:
                            // Receive new IIR-Filter-Config
                            receive_struct(sock_client, &liaIIRCfg, liaIIRCfgBuffer, sizeof(LiaIIRConfig));
                            printf("\n### Received new IIR-Filter-Coefficents ###\n");
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            load_lia_iir_coeffs(axi_devs, liaIIRCfg, verbose);
                            break;

                        case RAM_INIT_CONFIG_ID:
                            // Initialize RAM with config from host
                            receive_struct(sock_client, &ramInitCfg, ramInitCfgBuffer, sizeof(RamInitConfig));
                            printf("\n### Received new RAM-Init-Config ###\n");
                            ramCfg = init_ram(axi_devs, ramInitCfg);
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            break;

                        case CLK_DIVIDER_CONFIG_ID:
                            receive_struct(sock_client, &clockDividerConfig, ClockDividerConfigBuffer, sizeof(ClockDividerConfig));
                            printf("\n### Received new Clock_Divider-Config ###\n");
                            // config Clock_Divider with values from host:
                            config_clock_divider(axi_devs, clockDividerConfig);
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            break;

                        case SPI_CONFIG_ID:
                            receive_struct(sock_client, &spiCfg, spiCfgBuffer, sizeof(SpiConfig));
                            printf("\n### Received new SPI-Config ###\n");
                            spi_fd = setup_spi(spiCfg);
                            // Send ACK to show Host-PC that configuration is done
                            send_to_client(sock_client, CONFIG_DONE);
                            
                            break;

                        default:
                            printf("Config-ID %d not implemented yet...\n", (int)command.val);
                            break;
                    }
                    break;

                case SET_RP_DAC_NO_CALIB:
                    // Set raw voltage on RP-DAC (used when calibrating the DACs)
                    set_voltage_on_DAC_no_calib(axi_devs, RP_DAC_ID, command.ch, command.val, verbose);
                    break;

                case SET_RP_DAC:
                    // Set voltage on RP-DAC
                    set_voltage_on_DAC(axi_devs, RP_DAC_ID, command.ch, command.val, verbose);
                    break;

                case SET_AD_DAC:
                    // Set voltage on external AD-DAC
                    set_voltage_on_DAC(axi_devs, AD_DAC_ID, command.ch, command.val, verbose);
                    break;

                case GET_RF_ADC:
                    // Read voltage on fast ADC and send to the client
                    voltage_mV = (int)(rpa_get_voltage(axi_devs, command.ch, true, command.val) * 1000);
                    if (verbose) printf("\t voltage measured on fast ADC on channel %d : %d mV \n", command.ch, voltage_mV);
                    send_to_client(sock_client, voltage_mV);
                    break;

                case GET_RF_ADC_CNT:
                    // Read ADC count on RF-ADC channel and send to the client
                    adc_cnt = rpa_get_raw_value(axi_devs, command.ch);
                    if (verbose) printf("\t adc cnt on RF-ADC ch: %d : %x \n", command.ch, adc_cnt);
                    send_to_client(sock_client, adc_cnt);
                    break;

                case SET_PDM:
                    // Set voltage on PDM and enable PDM output
                    set_voltage_pdm(axi_devs, command.ch, command.val);
                    enable_pdm_output(axi_devs, command.ch);
                    break;

                case GET_XADC:
                    // Read XADC ports and return voltage for the selected channel
                    voltage_mV = (int)(xad_get_voltage(axi_devs, command.ch) * 1000);
                    if (verbose) printf("\t voltage measured: %d \n", voltage_mV);
                    send_to_client(sock_client, voltage_mV);
                    break;

                case START_DAC_SWEEP:
                    // Start DAC sweep for an infinite amount of time
                    // TODO: Use separated Resets so we can Config dac-bram-controllers independent from others while they are currently running
                    // enable module for synchronizing bram-ctrl:
                    enable_module(axi_devs, RESET_INDEX_BRAM_CTRL_SYNC);
                    start_bram_dac_output(axi_devs, command.ch);  // start bram-dac-output for selected BramDac-Port
                    printf("###### Started LUT-DAC-Output for port %d ######\n", command.ch);
                    break;

                case STOP_DAC_SWEEP:
                    // Stop DAC sweep, reset module, and exit application
                    printf("Laser-Tuning stopped...\n");
                    stop_bram_dac_output(axi_devs, command.ch);
                    break;

                case START_TRIGGER_SWEEP:
                    // entry point for DAC-Sweep with trigger-handshake for measuring wavelenght during a sweep
                    // for usage with trigger_generator-IP
                    // expects the bram-port-id as channel-value (command.ch)
                    disable_module(axi_devs,RESET_INDEX_TRIGGER_GEN);// disable before starting the other modules
                    trigger_sweep_index = 0; // reset sweep-index for a new sweep:
                    no_total_trigger    = command.val;
                    printf("Start Trigger-Sweep for bram-port: %d and %d trigger: ####\n",command.ch,no_total_trigger);
                    // enable needed modules:
                    enable_module(axi_devs, RESET_INDEX_DAC_BRAM_CTRL_PORT0);
                    enable_module(axi_devs, RESET_INDEX_BRAM_CTRL_SYNC);
                    enable_module(axi_devs, RESET_INDEX_TRIGGER_GEN);
                    // start output:
                    start_bram_dac_output(axi_devs, command.ch);
                    // wait till first trigger gets armed on FPGA-Logic
                    wait_for_wlength_request(axi_devs);
                    // now send request to Host-PC to read out measured wavelength at trigger
                    send_to_client(sock_client, REQ_WLENGTH);
                    printf("## Armed first trigger %d  for trigger-sweep with %d trigger:\n", (trigger_sweep_index + 1), no_total_trigger);
                    break;

                case NEXT_TRIGGER:
                    // make sure to release the trigger before selecting the next-trigger
                    // increment trigger-ref to select next trigger-timestep, clears check-wavelength
                    select_next_trigger(axi_devs);
                    printf("## Wavelength measured for %d/%d\n", (trigger_sweep_index + 1), no_total_trigger);
                    // wait for trigger beeing armed by FPGA-Logic
                    wait_for_wlength_request(axi_devs);
                    // now send request to Host-PC to read out measured wavelength at trigger
                    send_to_client(sock_client, REQ_WLENGTH);
                    printf("## Requested wavelength for next trigger %d/%d\n", (trigger_sweep_index + 2), no_total_trigger);
                    trigger_sweep_index++;
                    if ((trigger_sweep_index + 1) == no_total_trigger) {
                        printf("## Wavelength measured for last Trigger %d/%d \n", (trigger_sweep_index + 1), no_total_trigger);
                        printf("## Finished Trigger-Sweep! ############################ \n");
                    }
                    break;

                case REARM_TRIGGER:
                    // if wavelength-measurement failed or timed-out
                    // we want to measure the wavelength again for the same trigger, so we rearm it
                    rearm_current_trigger(axi_devs);
                    printf("### Rearmed current trigger %d/%d", (trigger_sweep_index + 1), no_total_trigger);
                    wait_for_wlength_request(axi_devs);
                    break;

                case HOLD_TRIGGER:
                    // holds output-trigger high at current-trigger point until we select-next trigger
                    // ..setting rearm-trigger constantly to 1
                    // so there is no need for rearming all the time
                    printf("### Hold current trigger %d \n",(trigger_sweep_index+1));
                    hold_current_trigger(axi_devs);
                    break;

                case RELEASE_TRIGGER:
                    // make sure to wait some time inbetween releasing the trigger and
                    // selecting the next trigger... the time you need to sleep
                    // depends on the DAC-SIGNAL-PERIOD (dac-dwell-time * dac-no-steps)
                    printf("Trigger %d got released",(trigger_sweep_index+1));
                    release_current_trigger(axi_devs);
                    send_to_client(sock_client,ACK);
                    break;

                case ADJ_LUT_VALUE:
                    //inputs: - which Bram-Port?... from that we have access to bramDacConfig through bramDacConfig_arr
                    printf("Received command to adjust LUT-Value:\n");
                    // receive new LUT-Value-Struct:
                    receive_struct(sock_client, &lutValue, LutValueBuffer, sizeof(LutValue));
                    // apply new LutValue:
                    change_value_in_lut(axi_devs, lutValue, verbose);

                    // for first value in LUT we also want to adjust the last value in LUT
                    if (lutValue.index == 0) {
                        change_value_in_lut_at_index(axi_devs, lutValue, bramDacConfig_arr[lutValue.port_id].no_steps - 1, verbose);
                    }
                    printf("Received adjusted tuning voltage: %fV for trigger %d \n", lutValue.voltage, (trigger_sweep_index + 1));
                    // now repeat wlength measurement for same trigger:
                    rearm_current_trigger(axi_devs);
                    // wait till trigger is armed by FPGA
                    wait_for_wlength_request(axi_devs);
                    // send wlength request to HOST-PC to measure wavelength at trigger:
                    send_to_client(sock_client, REQ_WLENGTH);
                    break;

                case STORE_LUT:
                    // we expect a port_id for the LUT/DAC-BRAM-Controller-Port we want to store the LUT (via channel-id)
                    store_adj_lut_to_file(axi_devs,bramDacConfig_arr[command.ch],verbose);
                    printf("Stored LUT for DAC-BRAM-CONRTOLLER at Port %d\n",command.ch);
                    break;

                case START_ADC_SAMPLING:
                    signal(SIGINT, SIG_DFL);  // so that we can go outside here if we need to interrupt...
                    printf("received start ADC-Sampling command...\n");
                    no_tcp_packages = (int)command.val;  // send amount of tcp package we wan to sample when sending startADC sampling request!
                    switch (adcCfg.adc_mode) {
                        case ADC_CONTINOUS_MODE:
                            /* continous mode for ADC-Sampling till 15.626 MS/s*/
                            printf("##### Start ADC-RAM-TCP-Writer in Continous-Mode for %d TCP-Packages #####\n", no_tcp_packages);
                            cont_adc_writer(axi_devs, sock_client, ramCfg, no_tcp_packages, adcCfg.sample_rate_divider, verbose);
                            break;
                        case ADC_BLOCK_MODE:
                            /* single shot block-mode with upto 125MS/s */
                            printf("##### Start ADC-RAM-TCP-Writer in Block-Mode for %d TCP-Packages #####\n", no_tcp_packages);
                            block_adc_writer(axi_devs, sock_client, ramCfg, verbose);
                            break;
                        case ADC_LIA_MODE:
                            /* apply LIA on sampled ADC data and send via tcp*/
                            printf("#### Start LIA-ADC-RAM-TCP-Writer for %d Blocks\n",no_tcp_packages);
                            //lia_adc_writer(axi_devs, sock_client, no_tcp_packages, ramCfg);
                        default:
                            printf("ADC-Mode not available...\n");
                            break;
                    }
                    printf("Done sampling ADC...\n");
                    disable_rp_adc(axi_devs);
                    disable_ram_writer(axi_devs);
                    signal(SIGINT, signal_handler);
                    break;

                case SET_LED:
                    turn_on_leds(axi_devs, (int) command.val);
                    break;

                case EXIT_APP:
                    printf("exit application...\n");
                    reset_system(axi_devs, true);  // forced reset on all modules...
                    close(sock_client);
                    close(sock_server);
                    // reset interrupt signal
                    signal(SIGINT, SIG_DFL);
                    goto server_shutdown;

                case TERMINATE_CLIENT:
                    // when receiving a terminate-client command from Python (executed before closing python-socket)
                    goto exit_client_connection;

                case CLIENT_DISCONNECT:
                    // when receiving invalid data we close client-socket
                    goto exit_client_connection;

                case GET_DAC_BRAM_SAMPLE_CNT:
                    // Read ADC count on RF-ADC channel and send to the client
                    sample_rate_cnt = get_sample_rate_bram_no_clocks(axi_devs, command.ch);
                    send_to_client(sock_client, sample_rate_cnt);
                    break;

                case GET_DAC_BRAM_SIGNAL_CNT:
                    // Read ADC count on RF-ADC channel and send to the client
                    signal_rate_cnt = get_signal_rate_bram_no_clocks(axi_devs, command.ch);
                    send_to_client(sock_client, signal_rate_cnt);
                    break;

                /**************************************************************/
                /* Test-Commands for Debugging and Testing                    */
                /**************************************************************/

                case DEBUG:
                    // add some debug-content... (reading back axi-config values etc..)
                    // or printing some additional information regarding current tests
                    // print configs for bram-dac-controllers:
                    for (int i = 0; i < NO_DAC_BRAM_INTERFACES_USED; i++) {
                        printf("### BRAM-DAC-Config-%d: ###\n", i);
                        printf("dwell-time-delay:%d\n", bramDacConfig_arr[i].dwell_time_delay);
                        printf("no-steps:%d\n", bramDacConfig_arr[i].no_steps);
                    }
                    break;

                /**************************************************************/
                /* Test-Commands for RAM Writer test project                  */
                /**************************************************************/

                case RAM_TEST_BLOCK_MODE:
                    // Test RAM-Writer in Block-Mode
                    test_ram(axi_devs, sock_client, ramCfg, RAM_WRITER_BLOCK_MODE, no_tcp_packages, verbose);
                    break;

                case RAM_TEST_CONTI_MODE:
                    // Test RAM-Writer in Continous-Mode, with a given amount of tcp-packages
                    no_tcp_packages = (int)command.val;
                    test_ram(axi_devs, sock_client, ramCfg, RAM_WRITER_CONTI_MODE, no_tcp_packages, verbose);
                    break;

                case START_CLK_DIV:
                    reset_clock_divider(axi_devs);
                    printf("Start Clock Divider\n");
                break;

                case STOP_CLK_DIV:
                    disable_clock_divider(axi_devs);
                    printf("Stop Clock Divider\n");
                break;

                /**************************************************************/
                /* Test-Commands for LIA debug project                        */
                /**************************************************************/

                case SELECT_RAM_MUX_INPUT:
                    // select input for AXIS-Multiplexer
                    select_ram_mux_input(axi_devs, command.val, verbose);
                    break;

                case SELECT_ADC_MUX_INPUT:
                    // select input for ADC-Multiplexer
                    select_adc_mux_input(axi_devs, command.val, verbose);
                    break;

                case LIA_DEBUG:
                    // Test LIA-Mixer-Module
                    lia_debug(axi_devs, sock_client, ramCfg, verbose);
                    break;

                case CLOSE_SPI:
                    release_spi(spi_fd);
                    break;

                /**********************************************************************/
                 /* Commands to read back voltage value from ADC Click Boards over SPI*/
                 /********************************************************************/

                 case SPI_TEST:
                    printf("## Testing SPI strted.....##\n");
                    spi_test();
                    break;

                 case INIT_ADC24:
                    // use this command at the very beginning before sending any other command to ADC24 Ctrl Register
                    //this command ensures that the next configuration (command) sent will be properly set
                    init_adc24(spi_fd);
                    printf("## Initialising the ADC24 Click Board##\n");
                    break;

                 case GET_VOLTAGE_ADC24:
                    // read the voltage from the selected channel of ADC24Click over SPI
                    voltage_mV = get_voltage_adc24(spi_fd, command.ch);
                    if (verbose) printf("\t voltage measured on channel %d : %dV \n", command.ch, voltage_mV);
                    send_to_client(sock_client, voltage_mV);
                    break;

                case START_SEQ_SAMPLING_ADC24:
                    printf("## Started ADC24 in sequence mode from Channel 0 to %d##\n", command.ch);
                    size = command.ch + 1;
                    ptr = sequence_mode_adc24(spi_fd, command.ch, size, verbose);
                    //send the array to client
                    send(sock_client, ptr, size * sizeof(int), MSG_NOSIGNAL);
                    free(ptr);
                    break;

                case INIT_ADC20: // make this a CONFIG_ID ... CONFIG_ADC_CLICKBOARD_20/24...
                    // this command sets sampling rate of ADC20 to 1MHz, configures the channels as analog inputs
                    init_adc20(spi_fd);
                    printf("## Initialising the ADC20 Click Board##\n");
                    //send_to_client(sock_client, CONFIG_DONE);
                    break;

                case GET_VOLTAGE_ADC20:
                    // reads the voltage from the selected ADC20Click channel over SPI
                    voltage_mV = get_voltage_adc20(spi_fd, command.ch, command.val);
                    printf("voltage measured on channel %d of ADC20: %dmV \n", command.ch, voltage_mV);
                    send_to_client(sock_client, voltage_mV);
                    break;

                case START_SEQ_SAMPLING_ADC20:
                    printf("## Started ADC20 in sequence mode starting from Channel 0 to %d##\n", command.ch);
                    size = command.ch + 1;
                    ptr = sample_sequence_mode_adc20(spi_fd, command.ch, command.val,  size,  verbose);
                    //send array to client
                    send(sock_client, ptr, size * sizeof(int), MSG_NOSIGNAL);
                    free(ptr);
                    break;

                case READ_REG_ADC20:
                    printf("## Read register value from %d ##\n", command.ch);
                    read_register(spi_fd,  command.ch);
                    break;
                
                case ADC20_DEBUG_CMD:
                    adc20_debug_communication(spi_fd);
                    break;

                /**************************************************************/
                /* Default-Case for not implemented commands                  */
                /**************************************************************/
                default:
                    printf("Command with ID %d is not implemented...\n", command.id);
                    break;

            }  // switch-case

        }      // wait-for-commands-loop

        exit_client_connection:
        close(sock_client);

    }  // wait-for-client-conncect-loop
    server_shutdown:
    close(sock_server);
    signal(SIGINT, SIG_DFL);
    printf("shutdown server...\n");
    return 0;
}
