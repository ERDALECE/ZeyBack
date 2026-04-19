

#set_property PACKAGE_PIN K2 [get_ports MCU_NRST]
#set_property IOSTANDARD LVCMOS33 [get_ports MCU_NRST]


set_property CONFIG_MODE SPIx4 [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 33 [current_design]









#set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets {clk98_ibuf}]






#set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets I2S_FPGA_BCKL_IBUF]
#create_clock -name i2s_bclk -period 40.690 [get_ports I2S_FPGA_BCKL]
#set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets I2S_FPGA_BCKL_IBUF]




set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]





set_property PACKAGE_PIN M2 [get_ports {STM32_ADDRESS[0]}]
set_property PACKAGE_PIN K3 [get_ports {STM32_ADDRESS[1]}]
set_property PACKAGE_PIN U12 [get_ports {SPDIF_XMOS_SEL[0]}]
set_property PACKAGE_PIN T13 [get_ports {SPDIF_XMOS_SEL[1]}]
set_property PACKAGE_PIN R11 [get_ports {SPDIF_XMOS_SEL[2]}]
set_property PACKAGE_PIN U13 [get_ports {SAMPLE_RATE[0]}]
set_property PACKAGE_PIN T14 [get_ports {SAMPLE_RATE[1]}]
set_property PACKAGE_PIN R12 [get_ports {SAMPLE_RATE[2]}]
set_property PACKAGE_PIN T15 [get_ports {SAMPLE_RATE[3]}]
set_property PACKAGE_PIN L1 [get_ports {STM32_DATA[31]}]
set_property PACKAGE_PIN L4 [get_ports {STM32_DATA[30]}]
set_property PACKAGE_PIN L3 [get_ports {STM32_DATA[29]}]
set_property PACKAGE_PIN M6 [get_ports {STM32_DATA[28]}]
set_property PACKAGE_PIN N4 [get_ports {STM32_DATA[27]}]
set_property PACKAGE_PIN U4 [get_ports {STM32_DATA[26]}]
set_property PACKAGE_PIN U3 [get_ports {STM32_DATA[25]}]
set_property PACKAGE_PIN V4 [get_ports {STM32_DATA[24]}]
set_property PACKAGE_PIN V5 [get_ports {STM32_DATA[23]}]
set_property PACKAGE_PIN U6 [get_ports {STM32_DATA[22]}]
set_property PACKAGE_PIN V6 [get_ports {STM32_DATA[21]}]
set_property PACKAGE_PIN T6 [get_ports {STM32_DATA[20]}]
set_property PACKAGE_PIN R3 [get_ports {STM32_DATA[19]}]
set_property PACKAGE_PIN P4 [get_ports {STM32_DATA[18]}]
set_property PACKAGE_PIN R6 [get_ports {STM32_DATA[17]}]
set_property PACKAGE_PIN N6 [get_ports {STM32_DATA[16]}]
set_property PACKAGE_PIN V9 [get_ports {STM32_DATA[15]}]
set_property PACKAGE_PIN U8 [get_ports {STM32_DATA[14]}]
set_property PACKAGE_PIN U9 [get_ports {STM32_DATA[13]}]
set_property PACKAGE_PIN T3 [get_ports {STM32_DATA[12]}]
set_property PACKAGE_PIN R8 [get_ports {STM32_DATA[11]}]
set_property PACKAGE_PIN R7 [get_ports {STM32_DATA[10]}]
set_property PACKAGE_PIN V1 [get_ports {STM32_DATA[9]}]
set_property PACKAGE_PIN U1 [get_ports {STM32_DATA[8]}]
set_property PACKAGE_PIN R1 [get_ports {STM32_DATA[7]}]
set_property PACKAGE_PIN T1 [get_ports {STM32_DATA[6]}]
set_property PACKAGE_PIN P2 [get_ports {STM32_DATA[5]}]
set_property PACKAGE_PIN R2 [get_ports {STM32_DATA[4]}]
set_property PACKAGE_PIN N2 [get_ports {STM32_DATA[3]}]
set_property PACKAGE_PIN R5 [get_ports {STM32_DATA[2]}]
set_property PACKAGE_PIN P3 [get_ports {STM32_DATA[1]}]
set_property PACKAGE_PIN U7 [get_ports {STM32_DATA[0]}]
set_property PACKAGE_PIN T16 [get_ports BR_MCU]
set_property PACKAGE_PIN T5 [get_ports Clock_SI]
set_property PACKAGE_PIN N5 [get_ports Clock_SI_50MHZ]
set_property PACKAGE_PIN E2 [get_ports COAX_IN_FPGA]
set_property PACKAGE_PIN G17 [get_ports DSD_A]
set_property PACKAGE_PIN G18 [get_ports DSD_A_N]
set_property PACKAGE_PIN J17 [get_ports DSD_B]
set_property PACKAGE_PIN J18 [get_ports DSD_B_N]
set_property PACKAGE_PIN F18 [get_ports DSD_CLK]
set_property PACKAGE_PIN H17 [get_ports DSD_EN]
set_property PACKAGE_PIN L5 [get_ports DSP_SEL]
set_property PACKAGE_PIN B2 [get_ports HDMI_DATA]
set_property PACKAGE_PIN A1 [get_ports HDMI_FCLK]
set_property PACKAGE_PIN B1 [get_ports HDMI_LRCLK]
set_property PACKAGE_PIN B3 [get_ports HDMI_MCLK]
set_property PACKAGE_PIN P17 [get_ports I2S_FPGA_BCKL]
set_property PACKAGE_PIN U18 [get_ports I2S_FPGA_DATA]
set_property PACKAGE_PIN T18 [get_ports I2S_FPGA_LRCLK]
set_property PACKAGE_PIN B17 [get_ports L_BCLK]
set_property PACKAGE_PIN B18 [get_ports L_CLK]
set_property PACKAGE_PIN C17 [get_ports L_DATA_N]
set_property PACKAGE_PIN A18 [get_ports L_DATA_P]
set_property PACKAGE_PIN V16 [get_ports LED1]
set_property PACKAGE_PIN U16 [get_ports LED2]
set_property PACKAGE_PIN V17 [get_ports LED3]
set_property PACKAGE_PIN R13 [get_ports MUTE]
set_property PACKAGE_PIN E1 [get_ports OPTICAL_IN_1]
set_property PACKAGE_PIN C1 [get_ports OPTICAL_IN_2]
set_property PACKAGE_PIN D18 [get_ports R_BCLK]
set_property PACKAGE_PIN E17 [get_ports R_CLK]
set_property PACKAGE_PIN E18 [get_ports R_DATA_N]
set_property PACKAGE_PIN D17 [get_ports R_DATA_P]
set_property PACKAGE_PIN R10 [get_ports RCLK_ON]
set_property PACKAGE_PIN G1 [get_ports SPDIF_CLK]
set_property PACKAGE_PIN H1 [get_ports SPDIF_DATA]
set_property PACKAGE_PIN U17 [get_ports SPDIF_LOCK]
set_property PACKAGE_PIN F1 [get_ports SPDIF_LRCLK]
set_property PACKAGE_PIN K2 [get_ports STM32_EN]
set_property PACKAGE_PIN N1 [get_ports STM32_NE1_N]
set_property PACKAGE_PIN V2 [get_ports STM32_NOE_N]
set_property PACKAGE_PIN M1 [get_ports STM32_NWAIT]
set_property PACKAGE_PIN U2 [get_ports STM32_NWE_N]
set_property PACKAGE_PIN K1 [get_ports STM32_REQ]
set_property PACKAGE_PIN V15 [get_ports XMOS_DATA]
set_property PACKAGE_PIN V12 [get_ports XMOS_DSDON]
set_property PACKAGE_PIN U14 [get_ports XMOS_FCLK]
set_property PACKAGE_PIN V14 [get_ports XMOS_LRCLK]
set_property PACKAGE_PIN V10 [get_ports XMOS_F0]
set_property PACKAGE_PIN T10 [get_ports XMOS_F1]
set_property PACKAGE_PIN V11 [get_ports XMOS_F2]
set_property PACKAGE_PIN U11 [get_ports XMOS_F3]

set_property PACKAGE_PIN H16 [get_ports Clock_SI2_1]
set_property PACKAGE_PIN E3 [get_ports Clock_SI2_2]
set_property PACKAGE_PIN P15 [get_ports Clock_SI3]
set_property PACKAGE_PIN B14 [get_ports OE_R_P]
set_property PACKAGE_PIN A14 [get_ports OE_R_N]
set_property PACKAGE_PIN A16 [get_ports OE_L_P]
set_property PACKAGE_PIN A15 [get_ports OE_L_N]


set_property PACKAGE_PIN T11 [get_ports {SPDIF_XMOS_SEL[3]}]

set_property PACKAGE_PIN R17 [get_ports RASPI_SPI_CLK]
set_property PACKAGE_PIN R18 [get_ports RASPI_SPI_CSN]
set_property PACKAGE_PIN R16 [get_ports RASPI_SPI_MISO]
set_property PACKAGE_PIN P18 [get_ports RASPI_SPI_MOSI]

set_property PACKAGE_PIN A13 [get_ports DSD_EN_RL]

set_property DRIVE 4 [get_ports R_BCLK]
set_property DRIVE 4 [get_ports R_CLK]
set_property DRIVE 4 [get_ports R_DATA_N]
set_property DRIVE 4 [get_ports R_DATA_P]
set_property DRIVE 4 [get_ports L_BCLK]
set_property DRIVE 4 [get_ports L_CLK]
set_property DRIVE 4 [get_ports L_DATA_N]
set_property DRIVE 4 [get_ports L_DATA_P]

set_property PULLTYPE PULLUP [get_ports {SPDIF_XMOS_SEL[3]}]
set_property PULLTYPE PULLUP [get_ports {SPDIF_XMOS_SEL[2]}]
set_property PULLTYPE PULLUP [get_ports {SPDIF_XMOS_SEL[1]}]
set_property PULLTYPE PULLUP [get_ports {SPDIF_XMOS_SEL[0]}]

set_property PULLTYPE PULLUP [get_ports L_BCLK]
set_property PULLTYPE PULLUP [get_ports L_CLK]
set_property PULLTYPE PULLUP [get_ports L_DATA_N]
set_property PULLTYPE PULLUP [get_ports L_DATA_P]
set_property PULLTYPE PULLDOWN [get_ports OE_L_N]
set_property PULLTYPE PULLDOWN [get_ports OE_L_P]
set_property PULLTYPE PULLDOWN [get_ports OE_R_N]
set_property PULLTYPE PULLDOWN [get_ports OE_R_P]
set_property PULLTYPE PULLUP [get_ports R_BCLK]
set_property PULLTYPE PULLUP [get_ports R_CLK]
set_property PULLTYPE PULLUP [get_ports R_DATA_N]
set_property PULLTYPE PULLUP [get_ports R_DATA_P]

set_property PULLTYPE PULLUP [get_ports STM32_EN]
set_property PULLTYPE PULLUP [get_ports STM32_NE1_N]
set_property PULLTYPE PULLUP [get_ports STM32_NOE_N]
set_property PULLTYPE PULLUP [get_ports STM32_NWAIT]
set_property PULLTYPE PULLUP [get_ports STM32_NWE_N]
set_property PULLTYPE PULLUP [get_ports STM32_REQ]

set_property PACKAGE_PIN M3 [get_ports STM32_DSD_ENB]

set_false_path -from [get_ports DSP_SEL]
set_false_path -from [get_ports HDMI_DATA]
set_false_path -from [get_ports HDMI_LRCLK]
set_false_path -from [get_ports I2S_FPGA_DATA]
set_false_path -from [get_ports MUTE]
set_false_path -from [get_ports RASPI_SPI_CLK]
set_false_path -from [get_ports I2S_FPGA_LRCLK]
set_false_path -from [get_ports SPDIF_DATA]
set_false_path -from [get_ports RCLK_ON]
set_false_path -from [get_ports RASPI_SPI_MOSI]
set_false_path -from [get_ports STM32_EN]
set_false_path -from [get_ports XMOS_DATA]
set_false_path -from [get_ports XMOS_LRCLK]
set_false_path -from [get_ports XMOS_DSDON]

#==============================================================================
# PLL Clock Definitions (Vivado otomatik oluþturabilir, ama explicit taným daha iyi)
#==============================================================================
# Ana giriþ clock
#create_clock -period 20.000 -name clk_si_50mhz [get_ports Clock_SI_50MHZ]

#==============================================================================
# PLL Çýkýþlarý - Ayný PLL'den geliyorlar, iliþkili clock'lar
#==============================================================================
# Vivado genellikle bunlarý otomatik tanýr, ama bazen explicit gerekir
# set_clock_groups ile iliþkiyi belirtin

# Seçenek A: Tüm PLL çýkýþlarý arasýnda false path (en basit)
set_false_path -from [get_clocks clk_out2_pll] -to [get_clocks clk_out3_pll]
set_false_path -from [get_clocks clk_out3_pll] -to [get_clocks clk_out2_pll]

# Seçenek B: Eðer 12MHz ve 50MHz arasý gerçekten veri geçiþi varsa
# ve senkronizer kullanýyorsanýz, max_delay ile gevþetin
# set_max_delay -from [get_clocks clk_out2_pll] -to [get_clocks clk_out3_pll] 20.0 -datapath_only











set_property DRIVE 4 [get_ports DSD_A]
set_property DRIVE 4 [get_ports DSD_A_N]
set_property DRIVE 4 [get_ports DSD_B]
set_property DRIVE 4 [get_ports DSD_B_N]
set_property DRIVE 4 [get_ports DSD_CLK]
set_property SLEW SLOW [get_ports DSD_A]
set_property SLEW SLOW [get_ports DSD_A_N]
set_property SLEW SLOW [get_ports DSD_B]
set_property SLEW SLOW [get_ports DSD_B_N]
set_property SLEW SLOW [get_ports DSD_CLK]

