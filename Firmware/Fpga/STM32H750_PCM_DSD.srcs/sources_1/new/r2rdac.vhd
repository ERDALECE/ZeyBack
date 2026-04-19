----------------------------------------------------------------------------------
-- Company: 
-- Engineer: Erdal TURKEKUL
-- 
-- Create Date: 13.02.2025 16:54:17
-- Design Name: 
-- Module Name: r2rdac - Behavioral
-- Project Name: r2rdac
-- Target Devices: Artix7-100
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------

LIBRARY IEEE;
USE  IEEE.STD_LOGIC_1164.all;
USE  IEEE.STD_LOGIC_ARITH.all;
USE  IEEE.STD_LOGIC_UNSIGNED.all;
use  IEEE.MATH_REAL.ALL;
library UNISIM;
use UNISIM.VComponents.all;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity r2rdac is

PORT
	(
	    Clock_SI2_1           : IN STD_LOGIC  ; 
	    Clock_SI2_2           : IN STD_LOGIC  ; 
	    Clock_SI3             : IN STD_LOGIC  ; 
        Clock_SI              : IN STD_LOGIC  ;
	    Clock_SI_50MHZ        : IN STD_LOGIC  ;
	 	    
		OPTICAL_IN_1         : IN STD_LOGIC  ;
		OPTICAL_IN_2         : IN STD_LOGIC  ;
		COAX_IN_FPGA         : IN STD_LOGIC  ;		
		
		XMOS_FCLK		     : IN STD_LOGIC  ;
		XMOS_LRCLK		     : IN STD_LOGIC  ;
		XMOS_DATA		     : IN STD_LOGIC  ;
		
		HDMI_FCLK		     : IN STD_LOGIC  ;
		HDMI_LRCLK		     : IN STD_LOGIC  ;
		HDMI_DATA		     : IN STD_LOGIC  ;
		HDMI_MCLK		     : IN STD_LOGIC  ;
	
     	XMOS_F0		         : IN STD_LOGIC  ;
		XMOS_F1   		     : IN STD_LOGIC  ;
		XMOS_F2  		     : IN STD_LOGIC  ;
		XMOS_F3  		     : IN STD_LOGIC  ;
		XMOS_DSDON 		     : IN STD_LOGIC  ;	
		--MCU_DSDON 		     : IN STD_LOGIC  ;
	   			
		SAMPLE_RATE          : OUT STD_LOGIC_VECTOR(3 DOWNTO 0):="0000";
		BR_MCU	             : OUT STD_LOGIC:='0';
		
		SPDIF_CLK		     : IN STD_LOGIC  ;		
		SPDIF_LRCLK		     : IN STD_LOGIC  ;
		SPDIF_DATA		     : IN STD_LOGIC  ;
		
		STM32_ADDRESS        : IN STD_LOGIC_VECTOR (1 DOWNTO 0);
		STM32_NE1_N          : IN STD_LOGIC  ;
        STM32_NOE_N          : IN STD_LOGIC  ;
        STM32_NWE_N          : IN STD_LOGIC  ;
        STM32_NWAIT          : OUT STD_LOGIC  ;
		STM32_EN		     : IN STD_LOGIC  ;
		STM32_DSD_ENB	     : IN STD_LOGIC  ;
		STM32_REQ		     : OUT STD_LOGIC  ;
		STM32_DATA		     : INOUT STD_LOGIC_VECTOR (31 DOWNTO 0); 
	    MUTE                 : IN STD_LOGIC  ;
		
		L_DATA_P		     : OUT STD_LOGIC ;
		L_DATA_N		     : OUT STD_LOGIC ;
		L_CLK   		     : OUT STD_LOGIC ;
		L_BCLK   		     : OUT STD_LOGIC ;
		
		R_DATA_P		     : OUT STD_LOGIC ;
		R_DATA_N			 : OUT STD_LOGIC ;
		R_CLK   		     : OUT STD_LOGIC ;
		R_BCLK   		     : OUT STD_LOGIC ;
		
		OE_R_P               : OUT STD_LOGIC ;
		OE_R_N               : OUT STD_LOGIC ;
		OE_L_P               : OUT STD_LOGIC ;
		OE_L_N               : OUT STD_LOGIC ;
		
		LED1                 : OUT STD_LOGIC ;
		LED2                 : OUT STD_LOGIC ;
		LED3                 : OUT STD_LOGIC ;
		SPDIF_LOCK           : OUT STD_LOGIC ;
		
		DSD_CLK              : OUT STD_LOGIC ;
		DSD_A                : OUT STD_LOGIC ;
		DSD_B                : OUT STD_LOGIC ;
		DSD_EN               : OUT STD_LOGIC ;
		DSD_A_N              : OUT STD_LOGIC ;
		DSD_B_N              : OUT STD_LOGIC ;
		DSD_EN_RL            : OUT STD_LOGIC:='1';
		
	    DSP_SEL              : IN STD_LOGIC:='0';
		RCLK_ON              : IN STD_LOGIC:='0';
	   		
	    SPDIF_XMOS_SEL	     : IN STD_LOGIC_VECTOR (3 DOWNTO 0); 
	    
	    I2S_FPGA_BCKL        : IN STD_LOGIC;
	    I2S_FPGA_LRCLK       : IN STD_LOGIC;
	    I2S_FPGA_DATA        : IN STD_LOGIC;
	    
	    --RASPI_CS_SEL         :  IN STD_LOGIC:='0';
	    RASPI_SPI_CLK        :  IN STD_LOGIC;
        RASPI_SPI_CSN        :  IN STD_LOGIC;
        RASPI_SPI_MOSI       :  IN STD_LOGIC;
        RASPI_SPI_MISO       :  OUT STD_LOGIC	
    
	);
end r2rdac;

architecture Behavioral of r2rdac is

constant sr_upd      : std_logic:='0';
constant samp_div_val  : INTEGER range 0 to 1000000 := 500000;

signal mhz50clk  : STD_LOGIC ;
signal mhz100clk : STD_LOGIC ;
signal mhz200clk : STD_LOGIC ;

signal xmos_in            : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal stm32_in           : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal spdif_in           : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal opt_in             : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal opt_1_in_spdif     : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal opt_2_in_spdif     : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal coax_in_spdif      : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal hdmi_in            : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal i2s_direct_in      : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal i2s_direct_out_rclk      : STD_LOGIC_VECTOR (2 DOWNTO 0);

signal i2s_out   : STD_LOGIC_VECTOR (2 DOWNTO 0);

signal shift_out                 : STD_LOGIC_VECTOR (8 DOWNTO 0);
signal shift_out_bitrate         : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal shift_out_bitrate_bufL    : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal shift_out_bitrate_bufR    : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal BR_L                      : STD_LOGIC ;
signal BR_R                      : STD_LOGIC ;

signal counter       : INTEGER range 0 to 2500 := 0;
signal Sampling_val  : INTEGER range 0 to 2500 := 0;

signal lrclk_div_val : INTEGER range 0 to 1000000 := 128;
signal bclk_div_val  : INTEGER range 0 to 1000000 := 2;

signal valid_bit_counter_L     : integer range 0 to 31 := 0;
signal valid_bit_counter_R     : integer range 0 to 31 := 0;
signal stable_bitrate_L        : std_logic := '0';
signal stable_bitrate_R        : std_logic := '0';
signal bitrate_timeout_counter : integer range 0 to 1000000 := 0;
constant BITRATE_TIMEOUT       : integer := 500000; -- 50MHz'de ~10ms timeout
constant VALID_THRESHOLD       : integer := 16;      -- 16 geerli bit sonrasnda onay

signal FCLK    : STD_LOGIC ;
signal LRCLK   : STD_LOGIC ;
signal DATA    : STD_LOGIC ;

signal SDATA    : STD_LOGIC ;
signal SCLK     : STD_LOGIC ;
signal BSYNC    : STD_LOGIC ;
signal SLRCLK   : STD_LOGIC ;

signal SDATA_OPT_1      : STD_LOGIC:='0';
signal SCLK_OPT_1       : STD_LOGIC:='0';
signal BSYNC_OPT_1      : STD_LOGIC:='0';
signal SLRCLK_OPT_1     : STD_LOGIC:='0';

signal SDATA_OPT_2      : STD_LOGIC:='0';
signal SCLK_OPT_2       : STD_LOGIC:='0';
signal BSYNC_OPT_2      : STD_LOGIC:='0';
signal SLRCLK_OPT_2     : STD_LOGIC:='0';

signal SDATA_OPT_MSB    : STD_LOGIC:='0';
signal SDATA_COAX_MSB   : STD_LOGIC:='0';

signal SPDIFCLK         : STD_LOGIC ;
signal AES3IN   		   : STD_LOGIC ; 
signal RESET_SPDIF      : STD_LOGIC ;
signal COAX_LOCK        : STD_LOGIC ;
signal OPTIC_LOCK_1     : STD_LOGIC ;
signal OPTIC_LOCK_2     : STD_LOGIC ;

signal  L_DATA_P_O	   :  STD_LOGIC ;
signal  L_DATA_N_O	   :  STD_LOGIC ;
signal  L_CLK_O         :  STD_LOGIC ;
signal  L_BCLK_O   		:  STD_LOGIC ;
		
signal  R_DATA_P_O	   :  STD_LOGIC ;
signal  R_DATA_N_O		:  STD_LOGIC ;
signal  R_CLK_O   		:  STD_LOGIC ;
signal  R_BCLK_O        :  STD_LOGIC ;

signal WSD   : STD_LOGIC ;
signal WSP   : STD_LOGIC ;
signal WSP1  : STD_LOGIC ;

signal LED  : STD_LOGIC ;
signal DSDON: STD_LOGIC ;
signal RESET_SAMP: STD_LOGIC:='0' ;

signal SPDIF_XMOS_SEL_sync  :  std_logic_vector (3 DOWNTO 0):="0000";
signal SPDIF_XMOS_SEL_sync1 :  std_logic_vector (3 DOWNTO 0):="0000";
signal SPDIF_XMOS_SEL_sync2 :  std_logic_vector (3 DOWNTO 0):="0000";

signal F_XMOS : STD_LOGIC_VECTOR (3 DOWNTO 0);
-----------------  DSP  --------------------------

 signal SamplingFRQ    : integer:=0 ;
 signal DSP_IN         : STD_LOGIC ; 
 signal DSP_OUT        : STD_LOGIC ;
 signal DSP_CLK_OUT    : STD_LOGIC ;
 signal DSP_LRCLK_OUT  : STD_LOGIC ;
 signal DSP_ENB        : STD_LOGIC:='0';
 signal DSP_active     : STD_LOGIC:='1';
 signal DSP_LR_Clk     : std_logic;

 signal reset_sig      : std_logic;
 signal rst_n_fpga     : std_logic;

 signal bit_cnt        : integer range 0 to 63 := 0;
 signal current_ch     : std_logic := '0';  -- '0' = L, '1' = R
 signal sign_L         : std_logic := '0';
 signal sign_R         : std_logic := '0';
  
 signal DATA_L         : std_logic := '0';
 signal DATA_R         : std_logic := '0';
 signal SRCLK          : std_logic := '0';
 signal RCLK_L         : std_logic := '0';
 signal RCLK_R         : std_logic := '0';

 signal RCLK_IN_SCK    : STD_LOGIC ; 
 signal RCLK_IN_LRCK   : STD_LOGIC ; 
 signal RCLK_IN_DATA   : STD_LOGIC ;
 signal RCLK_OUT_SCK   : STD_LOGIC ;
 signal RCLK_OUT_LRCK  : STD_LOGIC ;
 signal RCLK_OUT_DATA  : STD_LOGIC ;

 signal rst_n_audio  : std_logic;

 signal STM32_SR_Latch: std_logic_vector(2 downto 0);

 signal si_bclk : std_logic;
 signal bclk_ibuf :std_logic;
 signal stm_ack :std_logic;

 signal bclk_i : std_logic;
 signal ws_i :std_logic;
 signal data_i :std_logic;

 signal rclk_raspi_on :std_logic := '0';
 signal I2S_FPGA_BCKL_O :std_logic;
 signal rclk_raspi_on_dedect :std_logic;

  signal clr_active :std_logic := '0';
  signal clr_cnt : integer range 0 to 63 := 0;
 
  signal lrclk_det : std_logic;
 
  signal SAMPLE_RATE_buf : std_logic_vector (3 DOWNTO 0);
  signal DSD_EN_buf : std_logic;
  signal DSD_EN_RL_buf     : std_logic := '0';
  signal BR_MCU_buf : std_logic;
 
  signal STM32_dsd_clk : std_logic;
  signal STM32_dsd_a : std_logic;
  signal STM32_dsd_b : std_logic;
  signal STM32_dsd_a_n : std_logic;
  signal STM32_dsd_b_n : std_logic;
  signal STM32_dsd_en : std_logic;     
   
  signal WSP_prev : std_logic := '0'; 

  -- ========================================================
  -- DAC mute / silent-code controller (sys_clk = 50 MHz)
  -- Clears 74LVC595 with 24 zero bits + latch on:
  --   * power-up
  --   * manual MUTE
  --   * STM32_EN falling edge
  --   * PCM <-> DSD transition
  --   * sample-rate change
  --   * source-select change
  -- ========================================================
  constant SYS_CLK_HZ        : integer := 50000000;
  constant EVENT_HOLD_MS     : integer := 5;
  constant EVENT_HOLD_CYCLES : integer := (SYS_CLK_HZ / 1000) * EVENT_HOLD_MS;
  constant FS_TOL_DIV        : integer := 16;  -- about 6.25% tolerance

 type mute_state_t is (
    M_IDLE,
    M_SHIFT_HI,
    M_SHIFT_LO,
    M_LATCH_HI,
    M_LATCH_LO,
    M_HOLD_EVENT
);

  signal boot_pending      : std_logic := '1';
  signal mute_state        : mute_state_t := M_IDLE;
  signal hold_cnt          : integer range 0 to EVENT_HOLD_CYCLES := 0;
  signal shift_cnt         : integer range 0 to 23 := 0;

  signal mute_meta         : std_logic := '0';
  signal mute_sync         : std_logic := '0';
  signal mute_prev         : std_logic := '0';

  signal stm32_en_meta     : std_logic := '0';
  signal stm32_en_sync     : std_logic := '0';
  signal stm32_en_prev     : std_logic := '0';

  signal dsd_mode_async    : std_logic := '0';
  signal dsd_mode_meta     : std_logic := '0';
  signal dsd_mode_sync     : std_logic := '0';
  signal dsd_mode_prev     : std_logic := '0';

  signal lr_meta           : std_logic := '0';
  signal lr_sync           : std_logic := '0';
  signal lr_prev           : std_logic := '0';

  signal sel_prev          : std_logic_vector(3 downto 0) := (others => '0');

  signal lr_period_cnt     : integer range 0 to 65535 := 0;
  signal lr_period_prev    : integer range 0 to 65535 := 0;
  signal lr_period_valid   : std_logic := '0';

  signal mute_busy         : std_logic := '0';
  signal mute_hold         : std_logic := '0';
  signal dac_force_mute    : std_logic := '0';

  signal mute_L_BCLK       : std_logic := '0';
  signal mute_R_BCLK       : std_logic := '0';
  signal mute_L_CLK        : std_logic := '0';
  signal mute_R_CLK        : std_logic := '0';
  signal mute_DATA_L       : std_logic := '0';
  signal mute_DATA_R       : std_logic := '0';
  
  signal fs_change_cnt : integer range 0 to 2 := 0;
  
  
component pll
PORT
 (
  clk_in1  : IN STD_LOGIC  := '0';
  clk_out1 : OUT STD_LOGIC;	
  clk_out2 : OUT STD_LOGIC;	
  clk_out3 : OUT STD_LOGIC 		           
 );
end component;


begin
LED1 <= LED;
LED2<= not LED;
SPDIF_LOCK <= COAX_LOCK or OPTIC_LOCK_1 or OPTIC_LOCK_2 ;
rst_n_fpga <= not reset_sig;
--MCU_NRST<='1';
xmos_in(0)<=  XMOS_FCLK;	
xmos_in(1)<=  XMOS_LRCLK;
xmos_in(2)<=  XMOS_DATA;

spdif_in(0)<= SPDIF_CLK;	
spdif_in(1)<= SPDIF_LRCLK;
spdif_in(2)<= SPDIF_DATA;

hdmi_in(0)  <= HDMI_FCLK;	
hdmi_in(1)  <= HDMI_LRCLK;
hdmi_in(2)  <= HDMI_DATA;

coax_in_spdif(0)<=SCLK;
coax_in_spdif(1)<=SLRCLK;
coax_in_spdif(2)<=SDATA;--SDATA_COAX_MSB;

opt_1_in_spdif(0)<=SCLK_OPT_1;
opt_1_in_spdif(1)<=SLRCLK_OPT_1;
opt_1_in_spdif(2)<=SDATA_OPT_1;

opt_2_in_spdif(0)<=SCLK_OPT_2;
opt_2_in_spdif(1)<=SLRCLK_OPT_2;
opt_2_in_spdif(2)<=SDATA_OPT_2;

i2s_direct_in(0) <= I2S_FPGA_BCKL;
i2s_direct_in(1) <= I2S_FPGA_LRCLK;
i2s_direct_in(2) <= I2S_FPGA_DATA;

DSD_EN <=  (DSD_EN_buf or MUTE);-- and not XMOS_DSDON;
SAMPLE_RATE <=SAMPLE_RATE_BUF;

BR_MCU <= BR_MCU_buf;

u_ibuf_bclk : IBUF port map(I => Clock_SI, O => bclk_ibuf);

u_bufg_bclk : BUFG port map(I => bclk_ibuf, O => si_bclk);

mypll: pll 
  port map (
  clk_in1 => Clock_SI_50MHZ, 
  clk_out1 => mhz100clk, 
  clk_out2 => mhz50clk,
  clk_out3 => mhz200clk     
  );
  
reset: entity work.global_reset
    generic map (
        CLK_FREQ    => 50_000_000,  -- 50 MHz varsaylan clock
        RESET_TIME_MS  =>200       -- 200 ms reset sresi
    )
    port map(
        clk       => mhz50clk,
        power_on  => '1',  -- G aldnda '1' olan sinyal
        reset_out => reset_sig   -- Global reset k (aktif yksek)
   ); 
  
  
u_arst : entity work.audio_rst_stretch
  generic map(
    HOLD_CYCLES => 100000   -- ~1ms @98.304MHz (istersen dr)
  )
  port map(
    clk_ctrl  => mhz100clk,
    rst_n_in  => rst_n_fpga,
    kick      => sr_upd,
    rst_n_out => rst_n_audio
  );
  
u_rl_delay: entity work.relay_delay
  generic map(
        CLK_FREQ_HZ =>  100_000_000,
        DELAY_MS    => 500              -- 40 veya 50 yapabilirsiniz
    )
    port map (
        clk       => mhz100clk,
        rst       => rst_n_fpga,
        trig      => DSD_EN_buf,-- and not XMOS_DSDON, --DSD_EN_RL_Buf,--
        relay_out => DSD_EN_RL
    ); 
    
  
shift_o:  entity work.shift_reg
	port map 
	(
		CLK	=>   FCLK,
		D	=>	 i2s_out(2),
		Q	=>   shift_out
	); 

bitrt:  entity work.bitrate 
	port map
	(
		CLK	 => FCLK,
		D	 => i2s_out(2),
		Q    => shift_out_bitrate
	);
	
process(mhz50clk) 
begin
    if rising_edge(mhz50clk) then
          SPDIF_XMOS_SEL_sync1 <= SPDIF_XMOS_SEL; -- 2-stage synchronizer
		  SPDIF_XMOS_SEL_sync2 <= SPDIF_XMOS_SEL_sync1;
		  SPDIF_XMOS_SEL_sync <= SPDIF_XMOS_SEL_sync2;
    end if;
end process;


  
 raspi_spi: entity work.spi_status_slave 
  port map(
    clk    => mhz50clk,-- FPGA system clock (e.g. 50..200 MHz)
    rst_n  => rst_n_audio,  

    spi_sclk => RASPI_SPI_CLK,
    spi_csn  => RASPI_SPI_CSN, -- active-low
    spi_mosi => RASPI_SPI_MOSI,
    spi_miso => RASPI_SPI_MISO,

    -- status inputs (may be from other clock domains)
    fs_code  =>SAMPLE_RATE_buf,
    in_sel  => SPDIF_XMOS_SEL_sync,
    cs_sel  => '1',--RASPI_CS_SEL,
    dsd     =>  DSD_EN_buf,
    is24    => BR_MCU_buf,
    dsp_on    => DSP_SEL,
    reclk_on  => RCLK_ON,
    lock      => COAX_LOCK or OPTIC_LOCK_1 or OPTIC_LOCK_2,
    stream    => STM32_EN,
    mute      => MUTE,
    fifo_lvl  => "00000000",
    irq      => open -- optional: toggles on status change
  );
   
  
 stm32_fmc_bus: entity work.fmc32_to_i2s_dsd 
  generic map (
    FIFO_DEPTH =>65536,
    FIFO_DEPTH_TRSH =>4096,
    

    -- Capture robustness (sys_clk cycles):
    WR_CAP_DLY    => 2,      -- wait N cycles after write active before sampling D/A
    PAIR_WD_CYC   => 20000,  -- if L arrives but R doesn't, drop partial after this

    -- true  = take sample24 = D[23:0]   (LSB-aligned 24-bit in 32-bit container)
    -- false = take sample24 = D[31:8]   (MSB-aligned 24-bit)
    SAMPLE_LSB_ALIGNED => false
  )
  port map (
    rst_n      => rst_n_audio,
    sys_clk    => mhz100clk,

    -- Stream enable from MCU (flush FIFO on rising edge)
    en_stream  => STM32_EN,
    
    dsd_mode => STM32_DSD_ENB,

    -- FMC (SRAM-like) interface
    fmc_ne1_n => STM32_NE1_N, 
    fmc_noe_n  =>STM32_NOE_N, 
    fmc_nwe_n  =>STM32_NWE_N, 
    fmc_nwait  =>STM32_NWAIT,
    fmc_a     => STM32_ADDRESS,
    fmc_d     => STM32_DATA, 

    -- External BCLK from SI5340 (Fs*64)
    bclk_in    => si_bclk,

    -- I2S out
    i2s_bclk  => stm32_in(0),
    i2s_lrclk  => stm32_in(1),
    i2s_sd     => stm32_in(2),
    
    dsd_clk     =>  STM32_dsd_clk,
    dsd_a       =>  STM32_dsd_a, -- Left channel DSD data
    dsd_b       =>  STM32_dsd_b,-- Right channel DSD data
    dsd_a_n     =>  STM32_dsd_a_n,-- Left channel DSD data (inverted for diff)
    dsd_b_n     =>  STM32_dsd_b_n,-- Right channel DSD data (inverted for diff)
    dsd_en      =>  STM32_dsd_en,-- DSD enable (active high when DSD mode)

    -- debug
    dbg_stb   => open,
    dbg_cnt   => open
  );

  
rclk_raspi: entity work.i2s_reclock_raspi
  port map (
    -- I2S input from external source
   
    si_clk     => si_bclk,
    rst => reset_sig,
    reclk_enb    => rclk_raspi_on,
    
    -- Input I2S
    i2s_in_bclk  =>  i2s_direct_in(0),
    i2s_in_lrclk =>  i2s_direct_in(1),
    i2s_in_sdata =>  i2s_direct_in(2),
     
    -- Output I2S
    i2s_out_bclk  =>i2s_direct_out_rclk(0),
    i2s_out_lrclk =>i2s_direct_out_rclk(1),
    i2s_out_sdata =>i2s_direct_out_rclk(2)
    
  );

    
u_sampling: entity work.data_activity_blink
   generic map (
    DIV => samp_div_val   -- ihtiyaca gre deitir (daha hzl/yava)
  )
    port map (
        clk_ref => mhz50clk,
        data_in => DATA,
        rst  => '0', 
        led_out => LED
    );	
    
  
u_sr_dedect : entity work.sample_rate_detector_lock
  generic map(
    CLK_REF_HZ    => 50_000_000,
    MEAS_CYCLES   => 50_000,     -- 1ms
    LOCK_COUNT    => 3,
    TOGGLE_FACTOR => 2           -- LRCLK toggles 2 , BITCLK toogles 128
  )
  port map(
    i_clk_ref    => mhz50clk,
    i_rstb       => '0',
    i_sig        => lrclk_det,
    SamplingFRQ  => SamplingFRQ,
    SamplingFRQ_vec =>SAMPLE_RATE_BUF,
    o_lock       => open,
    o_valid_p    => open,
    o_toggles_dbg=> open
  );

     
 u_spdifin: entity work.aes3rx
   generic map (
      -- Registers width, determines minimal baud speed of input AES3 at given master clock frequency
      reg_width =>8
		)
	port map
	(
	  clk    => mhz200clk,		
      aes3   => COAX_IN_FPGA,     
      reset  => reset_sig, 
      i_clock_freq_int => SamplingFRQ, 	
      sdata  => SDATA,     
      sclk   => SCLK,      
      bsync  => open,     
      lrck   => SLRCLK,      
      active => COAX_LOCK
		
	);
	
u_opt_in_1: entity work.aes3rx
  generic map (
      -- Registers width, determines minimal baud speed of input AES3 at given master clock frequency
      reg_width =>6
		)
	port map
	(
	  clk    => mhz200clk,		
      aes3   => OPTICAL_IN_1,      
      reset  => reset_sig,  
      i_clock_freq_int => SamplingFRQ, 		
      sdata  => SDATA_OPT_1,     
      sclk   => SCLK_OPT_1,      
      bsync  => open,     
      lrck   => SLRCLK_OPT_1,      
      active => OPTIC_LOCK_1
		
	);	
	
u_opt_in_2: entity work.aes3rx
  generic map (
      -- Registers width, determines minimal baud speed of input AES3 at given master clock frequency
      reg_width =>6
		)
	port map
	(
	  clk    => mhz200clk,		
      aes3   => OPTICAL_IN_2,      
      reset  => reset_sig,  
      i_clock_freq_int => SamplingFRQ, 		
      sdata  => SDATA_OPT_2,     
      sclk   => SCLK_OPT_2,      
      bsync  => open,     
      lrck   => SLRCLK_OPT_2,      
      active => OPTIC_LOCK_2
		
	);	

	
u_bitratededect: process(FCLK)
begin
    if rising_edge(FCLK) then
        -- Timeout sayac
        if bitrate_timeout_counter < BITRATE_TIMEOUT then
            bitrate_timeout_counter <= bitrate_timeout_counter + 1;
        else
            -- Timeout oldu, bitrate geersiz
            stable_bitrate_L <= '0';
            stable_bitrate_R <= '0';
            valid_bit_counter_L <= 0;
            valid_bit_counter_R <= 0;
        end if;

        if (LRCLK = '1') then
            -- Sol kanal bitrate kontrol
            shift_out_bitrate_bufL <= shift_out_bitrate;
            
            -- 14-8 aras bitlerin tamam 0 m kontrol
            if (shift_out_bitrate_bufR(11 downto 4) = "00000000") then
                if valid_bit_counter_R > 0 then
                    valid_bit_counter_R <= valid_bit_counter_R - 1;
                end if;
            else
                -- Geerli bit tespit edildi, timeout'u sfrla
                bitrate_timeout_counter <= 0;
                
                if valid_bit_counter_R < VALID_THRESHOLD then
                    valid_bit_counter_R <= valid_bit_counter_R + 1;
                else
                    stable_bitrate_R <= '1';
                end if;
            end if;
            
                      
        else
            -- Sa kanal bitrate kontrol
            shift_out_bitrate_bufR <= shift_out_bitrate;
            
            -- 14-8 aras bitlerin tamam 0 m kontrol
            if (shift_out_bitrate_bufL(11 downto 4) = "00000000") then
                if valid_bit_counter_L > 0 then
                    valid_bit_counter_L <= valid_bit_counter_L - 1;
                end if;
            else
                -- Geerli bit tespit edildi, timeout'u sfrla
                bitrate_timeout_counter <= 0;
                
                if valid_bit_counter_L < VALID_THRESHOLD then
                    valid_bit_counter_L <= valid_bit_counter_L + 1;
                else
                    stable_bitrate_L <= '1';
                end if;
            end if;
            
          end if;
        
        BR_MCU_buf <= stable_bitrate_R or stable_bitrate_L;
        LED3 <= stable_bitrate_L or stable_bitrate_R;
    end if;
end process;	

                     i2s_out <= stm32_in when SPDIF_XMOS_SEL_sync="0000" else
		             spdif_in when SPDIF_XMOS_SEL_sync="0001" else
		             spdif_in when SPDIF_XMOS_SEL_sync="0010" else 
		             spdif_in when SPDIF_XMOS_SEL_sync="0011" else 
		             hdmi_in when SPDIF_XMOS_SEL_sync= "0100" else
		             xmos_in when SPDIF_XMOS_SEL_sync="0101" else					
					 coax_in_spdif when SPDIF_XMOS_SEL_sync="0110" else 
		             opt_1_in_spdif when SPDIF_XMOS_SEL_sync="0111" else 
				     opt_2_in_spdif when SPDIF_XMOS_SEL_sync="1000" else
					 i2s_direct_out_rclk  when SPDIF_XMOS_SEL_sync="1001" else "000";
					
					 --i2s_out <=  spdif_in;
					 			 
				rclk_raspi_on <= '1' when SPDIF_XMOS_SEL_sync="1001" else '0';
				rclk_raspi_on_dedect<= i2s_direct_in(0) when SPDIF_XMOS_SEL_sync="1001" else i2s_out(0);
				lrclk_det <= i2s_direct_in(1) when SPDIF_XMOS_SEL_sync="1001" else i2s_out(1);	 
		 
				DSD_EN_buf  <= '0' when (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="0101" ) or (STM32_dsd_en ='1' and SPDIF_XMOS_SEL_sync="0000" ) else '1';--or ( MUTE='1' and SPDIF_XMOS_SEL_sync="101")) else '1'  ;
				
	            dsd_mode_async <= '1' when
                    ((XMOS_DSDON = '1') and (SPDIF_XMOS_SEL_sync = "0000")) or
                    ((STM32_dsd_en = '1') and (SPDIF_XMOS_SEL_sync = "0101"))
                else
                    '0';

    dac_force_mute <= '1' when (mute_busy = '1' or mute_hold = '1') else '0';			
				
dac_mute_controller : process(mhz50clk)
    variable start_event : std_logic;
    variable period_now  : integer;
    variable diff        : integer;
    variable tol         : integer;
begin
    if rising_edge(mhz50clk) then

        -- varsayýlanlar: pulse sinyalleri normalde 0
        mute_L_BCLK <= '0';
        mute_R_BCLK <= '0';
        mute_L_CLK  <= '0';
        mute_R_CLK  <= '0';
        mute_DATA_L <= '0';
        mute_DATA_R <= '0';

        if rst_n_audio = '0' then
            boot_pending     <= '1';
            mute_state       <= M_IDLE;
            hold_cnt         <= 0;
            shift_cnt        <= 0;

            mute_meta        <= '0';
            mute_sync        <= '0';
            mute_prev        <= '0';

            stm32_en_meta    <= '0';
            stm32_en_sync    <= '0';
            stm32_en_prev    <= '0';

            dsd_mode_meta    <= '0';
            dsd_mode_sync    <= '0';
            dsd_mode_prev    <= '0';

            lr_meta          <= '0';
            lr_sync          <= '0';
            lr_prev          <= '0';

            sel_prev         <= (others => '0');

            lr_period_cnt    <= 0;
            lr_period_prev   <= 0;
            lr_period_valid  <= '0';
            fs_change_cnt    <= 0;

            mute_busy        <= '0';
            mute_hold        <= '0';

        else
            -- 2FF synchronizer'lar
            mute_meta     <= MUTE;
            mute_sync     <= mute_meta;

            stm32_en_meta <= STM32_EN;
            stm32_en_sync <= stm32_en_meta;

            dsd_mode_meta <= dsd_mode_async;
            dsd_mode_sync <= dsd_mode_meta;

            lr_meta       <= lrclk_det;
            lr_sync       <= lr_meta;

            start_event := '0';

            ----------------------------------------------------------------
            -- Event detection sadece IDLE durumunda çalýþsýn
            ----------------------------------------------------------------
            if mute_state = M_IDLE then

                -- 1) power-up
                if boot_pending = '1' then
                    start_event  := '1';
                    boot_pending <= '0';
                end if;

                -- 2) manuel MUTE rising edge
                if (mute_sync = '1') and (mute_prev = '0') then
                    start_event := '1';
                end if;

                -- 3) STM32_EN falling edge (sadece STM32 source aktifken)
                if (SPDIF_XMOS_SEL_sync = "0101") and
                   (stm32_en_prev = '1') and (stm32_en_sync = '0') then
                    start_event := '1';
                end if;

                -- 4) PCM <-> DSD geçiþi
                if dsd_mode_sync /= dsd_mode_prev then
                    start_event     := '1';
                    lr_period_valid <= '0';
                    lr_period_cnt   <= 0;
                    fs_change_cnt   <= 0;
                end if;

                -- 5) source select deðiþimi
                if SPDIF_XMOS_SEL_sync /= sel_prev then
                    start_event     := '1';
                    lr_period_valid <= '0';
                    lr_period_cnt   <= 0;
                    fs_change_cnt   <= 0;
                end if;

                -- 6) sample-rate detect (yalnýz PCM modunda)
                if dsd_mode_sync = '0' then
                    if (lr_prev = '0') and (lr_sync = '1') then
                        period_now := lr_period_cnt;
                        lr_period_cnt <= 0;

                        if period_now > 20 then
                            if lr_period_valid = '1' then
                                diff := abs(period_now - lr_period_prev);
                                tol  := (lr_period_prev / FS_TOL_DIV) + 2;

                                if diff > tol then
                                    if fs_change_cnt = 2 then
                                        start_event   := '1';
                                        fs_change_cnt <= 0;
                                    else
                                        fs_change_cnt <= fs_change_cnt + 1;
                                    end if;
                                else
                                    fs_change_cnt <= 0;
                                end if;
                            else
                                lr_period_valid <= '1';
                                fs_change_cnt   <= 0;
                            end if;

                            lr_period_prev <= period_now;
                        end if;
                    else
                        if lr_period_cnt < 65535 then
                            lr_period_cnt <= lr_period_cnt + 1;
                        end if;
                    end if;
                else
                    lr_period_valid <= '0';
                    lr_period_cnt   <= 0;
                    fs_change_cnt   <= 0;
                end if;

            else
                -- busy / hold sýrasýnda yeni event arama
                lr_period_cnt   <= 0;
                lr_period_valid <= '0';
                fs_change_cnt   <= 0;
            end if;

            ----------------------------------------------------------------
            -- FSM
            ----------------------------------------------------------------
            case mute_state is

                when M_IDLE =>
                    mute_busy <= '0';
                    mute_hold <= '0';

                    if start_event = '1' then
                        mute_busy  <= '1';
                        shift_cnt  <= 0;
                        mute_state <= M_SHIFT_HI;
                    end if;

                when M_SHIFT_HI =>
                    mute_busy   <= '1';
                    mute_hold   <= '0';
                    mute_DATA_L <= '0';
                    mute_DATA_R <= '0';
                    mute_L_BCLK <= '1';
                    mute_R_BCLK <= '1';
                    mute_state  <= M_SHIFT_LO;

                when M_SHIFT_LO =>
                    mute_busy   <= '1';
                    mute_hold   <= '0';
                    mute_DATA_L <= '0';
                    mute_DATA_R <= '0';

                    if shift_cnt = 23 then
                        mute_state <= M_LATCH_HI;
                    else
                        shift_cnt  <= shift_cnt + 1;
                        mute_state <= M_SHIFT_HI;
                    end if;

                when M_LATCH_HI =>
                    mute_busy  <= '1';
                    mute_hold  <= '0';
                    mute_L_CLK <= '1';
                    mute_R_CLK <= '1';
                    mute_state <= M_LATCH_LO;

                when M_LATCH_LO =>
                    mute_busy  <= '0';
                    mute_hold  <= '1';
                    hold_cnt   <= EVENT_HOLD_CYCLES;
                    mute_state <= M_HOLD_EVENT;

                when M_HOLD_EVENT =>
                    mute_busy <= '0';
                    mute_hold <= '1';

                    if hold_cnt = 0 then
                        mute_hold  <= '0';
                        mute_state <= M_IDLE;
                    else
                        hold_cnt <= hold_cnt - 1;
                    end if;

            end case;

            -- prev register'lar
            mute_prev     <= mute_sync;
            stm32_en_prev <= stm32_en_sync;
            dsd_mode_prev <= dsd_mode_sync;
            lr_prev       <= lr_sync;
            sel_prev      <= SPDIF_XMOS_SEL_sync;
        end if;
    end if;
end process;
				
	
u_dsd_out: process(XMOS_DSDON, STM32_dsd_en, SPDIF_XMOS_SEL_sync,
                   XMOS_FCLK, XMOS_LRCLK, XMOS_DATA,
                   STM32_dsd_clk, STM32_dsd_a, STM32_dsd_b, STM32_dsd_a_n, STM32_dsd_b_n,
                   i2s_out, DATA_L, DATA_R, RCLK_L, RCLK_R,
                   mute_busy, mute_hold,FCLK,LRCLK, DATA,
                   mute_L_BCLK, mute_R_BCLK, mute_L_CLK, mute_R_CLK, mute_DATA_L, mute_DATA_R)
     begin    
     
       if (mute_busy = '1') then
       
          OE_R_P      <= '0';
		  OE_R_N      <= '0';
		  OE_L_P      <= '0';
		  OE_L_N      <= '0';
        
          DSD_CLK <= '0';
          DSD_A   <= '0';
          DSD_B   <= '0';
          DSD_A_N <= '0';
          DSD_B_N <= '0';

        
          L_BCLK   <= mute_L_BCLK;
          R_BCLK   <= mute_R_BCLK;
          L_CLK    <= mute_L_CLK;
          R_CLK    <= mute_R_CLK;
          L_DATA_P <= mute_DATA_L;
          L_DATA_N <= mute_DATA_L;
          R_DATA_P <= mute_DATA_R;
          R_DATA_N <= mute_DATA_R;   

      elsif (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="0101")then        
        
         OE_R_P      <= '0';
		 OE_R_N      <= '0';
		 OE_L_P      <= '0';
		 OE_L_N      <= '0';
		 
         DSD_CLK <= XMOS_FCLK;
         DSD_A   <= (XMOS_LRCLK xor '1');
         DSD_B   <= (XMOS_LRCLK xor '0');
         DSD_A_N <= (XMOS_DATA xor '1');
         DSD_B_N <= (XMOS_DATA xor '0');
         FCLK    <= '0';
         LRCLK   <= '0';
         DATA<= '0';    	
          bclk_i <= '0';
          ws_i <='0';
          data_i <='0' ; 
          
          L_BCLK   <= mute_L_BCLK;
          R_BCLK   <= mute_R_BCLK;
          L_CLK    <= mute_L_CLK;
          R_CLK    <= mute_R_CLK;
          L_DATA_P <= mute_DATA_L;
          L_DATA_N <= mute_DATA_L;
          R_DATA_P <= mute_DATA_R;
          R_DATA_N <= mute_DATA_R;   
         
        elsif (STM32_dsd_en ='1' and SPDIF_XMOS_SEL_sync="0000" ) then
        
         OE_R_P      <= '0';
		 OE_R_N      <= '0';
		 OE_L_P      <= '0';
		 OE_L_N      <= '0';
        
         DSD_CLK <= STM32_dsd_clk;
         DSD_A   <= STM32_dsd_a;
         DSD_B   <= STM32_dsd_b;
         DSD_A_N <= STM32_dsd_a_n;
         DSD_B_N <= STM32_dsd_b_n;
         FCLK    <= '0';
         LRCLK   <= '0';
          DATA<= '0';    	
          bclk_i <= '0';
          ws_i <='0';
          data_i <='0' ;     
          
          L_BCLK   <= mute_L_BCLK;
          R_BCLK   <= mute_R_BCLK;
          L_CLK    <= mute_L_CLK;
          R_CLK    <= mute_R_CLK;
          L_DATA_P <= mute_DATA_L;
          L_DATA_N <= mute_DATA_L;
          R_DATA_P <= mute_DATA_R;
          R_DATA_N <= mute_DATA_R;
        
        else
        
          DSD_CLK <= '0';
	      DSD_A   <= '0';
		  DSD_B   <= '0';
		  DSD_A_N <= '0';
		  DSD_B_N <= '0';
	      FCLK    <= i2s_out(0);
		  LRCLK   <= i2s_out(1); 
		  DATA<= i2s_out(2);    	
          bclk_i <= FCLK;
          ws_i <= LRCLK;
          data_i <=i2s_out(2) ;  
         
          -- Continuous hookups (keep combinational; avoid latches/glitches)
          L_BCLK   <= not FCLK;
          R_BCLK   <= not FCLK;
 
  -- If your N-side is a complementary ladder, change to: not DATA_L / not DATA_R
          L_DATA_P <= DATA_L;
          L_DATA_N <= DATA_L;
          R_DATA_P <= DATA_R;
          R_DATA_N <= DATA_R;

          L_CLK    <= RCLK_L;
          R_CLK    <= RCLK_R;
          
          OE_R_P      <= '0';
		  OE_R_N      <= '0';
		  OE_L_P      <= '0';
		  OE_L_N      <= '0';
       
       end if;   
  end process;		
   
                         	
  -- ========================================================
  -- 74LVC595 drive from I2S stream (BCLK/LRCLK/DATA)
  --  * Captures 24 bits per channel (I2S 1-bit delay respected).
  --  * Pulses L_CLK/R_CLK (RCLK) after 24 bits (bit_cnt=23).
  --  * On STM32_EN rising edge, clears both 595 chains to zero once to avoid startup white noise.
  -- ========================================================
         
 

  sign_magnitude : process(bclk_i)
  begin
    if rising_edge(bclk_i) then
      if rst_n_audio = '0' then
        DATA_L      <= '0';
        DATA_R      <= '0';
        RCLK_L      <= '0';
        RCLK_R      <= '0';
        bit_cnt     <= 0;
        current_ch  <= '0';
        WSD         <= '0';
        WSP         <= '0';
        clr_active  <= '0';
        clr_cnt     <= 0;
   		STM32_REQ    <= '1';
      else
        -- one-cycle pulses; default low
        RCLK_L <= '0';
        RCLK_R <= '0';

        -- keep previous WS/EN for edge detection (old values used below)
        if (SPDIF_XMOS_SEL_sync="0000") then
        WSD <= ws_i;
        WSP <= STM32_EN and not MUTE;
        
        else
         WSD <= ws_i;
         WSP <= '1';
 		 STM32_REQ    <= '0';
        end if;

       if WSP = '0' then
        DATA_L     <= '0';
        DATA_R     <= '0';
        bit_cnt    <= 0;
        current_ch <= ws_i;
        clr_active <= '0';
        clr_cnt    <= 0;
        WSP_prev   <= WSP;
      else
        -- rising edge of stream enable => clear once
        if (WSP='1' and WSP_prev='0') then
          clr_active <= '1';
          clr_cnt    <= 0;
          bit_cnt    <= 0;
          current_ch <= ws_i;
        end if;
        WSP_prev <= WSP;

          if clr_active = '1' then
            DATA_L <= '0';
            DATA_R <= '0';
            if clr_cnt = 24 then
              RCLK_L     <= '1';
              RCLK_R     <= '1';
              clr_active <= '0';
              clr_cnt    <= 0;
              bit_cnt    <= 0;
              current_ch <= ws_i;
            else
              clr_cnt <= clr_cnt + 1;
            end if;

          else
            -- WS transition: reset bit counter and skip capture this cycle
            if ws_i /= WSD then
              current_ch <= ws_i;
              bit_cnt    <= 0;
            else
              if bit_cnt < 25 then
                if current_ch = '0' then
                  DATA_L <= data_i;
                else
                  DATA_R <= data_i;
                end if;

                if bit_cnt = 24 then
                  if current_ch = '0' then
                    RCLK_L <= '1';
                  else
                    RCLK_R <= '1';
                  end if;
                end if;

                bit_cnt <= bit_cnt + 1;
              else
                -- padding bits (e.g. 32-bit slot): keep inputs quiet
                if current_ch = '0' then
                  DATA_L <= '0';
                else
                  DATA_R <= '0';
                end if;
              end if;
            end if;
          end if;
        end if;
      end if;
    end if;
  end process sign_magnitude;
  
end Behavioral;

  
