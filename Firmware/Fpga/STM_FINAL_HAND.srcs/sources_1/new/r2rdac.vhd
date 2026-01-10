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
	    --Clock		         : IN STD_LOGIC  ;
	    Clock_24_576         : IN STD_LOGIC  ; 
	    Clock_22_5792        : IN STD_LOGIC  ; 
      --  ULPI_24MHZ		     : IN STD_LOGIC  ;
        Clock_SI              : IN STD_LOGIC  ;
	    Clock_SI_50MHZ        : IN STD_LOGIC  ;
	  --  Clock_SI_90_2       : IN STD_LOGIC  ;
	    
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
	
     	MCU_NRST 	         : OUT STD_LOGIC ;
	    		
		XMOS_F0		         : IN STD_LOGIC  ;
		XMOS_F1   		     : IN STD_LOGIC  ;
		XMOS_F2  		     : IN STD_LOGIC  ;
		XMOS_F3  		     : IN STD_LOGIC  ;
		XMOS_DSDON 		     : IN STD_LOGIC  ;	
		--MCU_DSDON 		     : IN STD_LOGIC  ;
	   			
		XMOS_F0_MCU	         : OUT STD_LOGIC:='0';
		XMOS_F1_MCU 	     : OUT STD_LOGIC:='0';
		XMOS_F2_MCU 	     : OUT STD_LOGIC:='0';
		XMOS_F3_MCU 	     : OUT STD_LOGIC:='0';
		--XMOS_DSDON_MCU	     : OUT STD_LOGIC:='0';	
	    BR_MCU	             : OUT STD_LOGIC:='0';
		--CLK_MCU	           : OUT STD_LOGIC:='0';
		
		SPDIF_CLK		     : IN STD_LOGIC  ;		
		SPDIF_LRCLK		     : IN STD_LOGIC  ;
		SPDIF_DATA		     : IN STD_LOGIC  ;
		
		STM32_ACK            : IN STD_LOGIC  ;
		STM32_VALID		     : IN STD_LOGIC  ;		
		STM32_EN		     : IN STD_LOGIC  ;
		STM32_REQ		     : OUT STD_LOGIC  ;
		STM32_DATA		     : IN STD_LOGIC_VECTOR (7 DOWNTO 0); 
	--	STM32_SR		     : IN STD_LOGIC_VECTOR (3 DOWNTO 0);
	    SR_FAM_IN            : IN STD_LOGIC  ;
		--SR_M2_IN		     : IN STD_LOGIC  ;
		--SR_M1_IN		     : IN STD_LOGIC  ;
		--SR_M0_IN		     : IN STD_LOGIC  ;
		STM32_SR_STB         : IN STD_LOGIC  ; --PA6
				
		L_DATA_P		     : OUT STD_LOGIC ;
		L_DATA_N		     : OUT STD_LOGIC ;
		L_CLK   		     : OUT STD_LOGIC ;
		L_BCLK   		     : OUT STD_LOGIC ;
		
		R_DATA_P		     : OUT STD_LOGIC ;
		R_DATA_N			 : OUT STD_LOGIC ;
		R_CLK   		     : OUT STD_LOGIC ;
		R_BCLK   		     : OUT STD_LOGIC ;
		
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
		
	    DSP_SEL              : IN STD_LOGIC:='0';
		RCLK_ON              : IN STD_LOGIC:='0';
	   		
	    SPDIF_XMOS_SEL	     : IN STD_LOGIC_VECTOR (2 DOWNTO 0); 
	    
	    STM32_EN_TEST         : OUT STD_LOGIC ;
	    STM32_CLK_TEST        : OUT STD_LOGIC ;
	    I2S_LRCLK_TEST        : OUT STD_LOGIC ;
	    I2S_BCLK_TEST         : OUT STD_LOGIC 
		);
end r2rdac;

architecture Behavioral of r2rdac is

signal mhz12clk  : STD_LOGIC ;
signal mhz50clk  : STD_LOGIC ;
signal mhz100clk : STD_LOGIC ;
signal mhz200clk : STD_LOGIC ;

signal xmos_in            : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal stm32_in            : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal spdif_in           : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal opt_in             : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal opt_1_in_spdif     : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal opt_2_in_spdif     : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal coax_in_spdif      : STD_LOGIC_VECTOR (2 DOWNTO 0);
signal hdmi_in            : STD_LOGIC_VECTOR (2 DOWNTO 0);

signal i2s_out   : STD_LOGIC_VECTOR (2 DOWNTO 0);

signal shift_out                 : STD_LOGIC_VECTOR (8 DOWNTO 0);
signal shift_out_bitrate         : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal shift_out_bitrate_bufL    : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal shift_out_bitrate_bufR    : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal BR_L                      : STD_LOGIC ;
signal BR_R                      : STD_LOGIC ;

signal counter       : INTEGER range 0 to 2500 := 0;
signal Sampling_val  : INTEGER range 0 to 2500 := 0;
signal samp_div_val  : INTEGER range 0 to 20000000 := 200000;
signal lrclk_div_val : INTEGER range 0 to 1000000 := 128;
signal bclk_div_val  : INTEGER range 0 to 1000000 := 2;

signal valid_bit_counter_L     : integer range 0 to 31 := 0;
signal valid_bit_counter_R     : integer range 0 to 31 := 0;
signal stable_bitrate_L        : std_logic := '0';
signal stable_bitrate_R        : std_logic := '0';
signal bitrate_timeout_counter : integer range 0 to 1000000 := 0;
constant BITRATE_TIMEOUT       : integer := 500000; -- 50MHz'de ~10ms timeout
constant VALID_THRESHOLD       : integer := 16;      -- 16 geçerli bit sonrasýnda onay

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

signal SPDIF_XMOS_SEL_sync : std_logic_vector(2 downto 0) := "000";
signal SPDIF_XMOS_SEL_sync1 : std_logic_vector(2 downto 0) := "000";
signal SPDIF_XMOS_SEL_sync2 : std_logic_vector(2 downto 0) := "000";

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

signal Clock_SI_Diff : std_logic;

signal dbg_stb_fpga   : std_logic;
signal dbg_l_msb_fpga : std_logic;

signal clk98_ibuf  : std_logic;
signal clk90_ibuf  : std_logic;
signal clk_audio   : std_logic;

signal fam_sel     : std_logic;
signal rate_code   : std_logic_vector(2 downto 0);
signal sr_upd      : std_logic;

signal rst_n_audio  : std_logic;

signal STM32_SR_Latch: std_logic_vector(2 downto 0);

signal si_bclk : std_logic;
signal bclk_ibuf :std_logic;
signal stm_ack :std_logic;

signal bclk_i : std_logic;
signal ws_i :std_logic;
signal data_i :std_logic;

component pll
PORT
 (
  clk_in1  : IN STD_LOGIC  := '0';
  clk_out1 : OUT STD_LOGIC;	
  clk_out2 : OUT STD_LOGIC;	
  clk_out3 : OUT STD_LOGIC;
  clk_out4 : OUT STD_LOGIC		           
 );
end component;

component shift_reg
    Port ( D   : in  STD_LOGIC;
           CLK : in  STD_LOGIC;
           Q : out STD_LOGIC_VECTOR (8 downto 0));
end component;

component clkDiv
    Port (
        clk_div: in  integer range 0 to 20000000 := 0;
		  clk_in : in  STD_LOGIC;
        reset  : in  STD_LOGIC;
        clk_out: out STD_LOGIC
    );
end component;

component bitrate
	PORT
	(
		CLK		: IN STD_LOGIC ;
		D		: IN STD_LOGIC ;
		Q		: OUT STD_LOGIC_VECTOR (31 DOWNTO 0)
	);
END component;

component sample_dedect
port (
  i_clk_ref            : in  std_logic;
  i_clk_test           : in  std_logic;
  i_rstb               : in  std_logic;
  o_clock_freq         : out std_logic_vector(15 downto 0);
  o_clock_freq_int     : out INTEGER range 0 to 1500 := 0
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

--stm32_in(0)<=  MCU_I2S3_FCLK;	
--stm32_in(1)<=  MCU_I2S_LRCLK;
--stm32_in(2)<=  MCU_I2S3_DATA;

--stm32_in(0)<=  STM32_CLK;	
--stm32_in(1)<=  STM32_LRCLK;
--stm32_in(2)<=  STM32_DATA;

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
opt_1_in_spdif(1)<=SDATA_OPT_1;
opt_1_in_spdif(2)<=SDATA_OPT_1;

opt_2_in_spdif(0)<=SCLK_OPT_2;
opt_2_in_spdif(1)<=SDATA_OPT_2;
opt_2_in_spdif(2)<=SDATA_OPT_2;

I2S_LRCLK_TEST    <= stm32_in(1);--LRCLK
I2S_BCLK_TEST     <= stm32_in(0);--FCLK;
STM32_EN_TEST     <= stm32_in(2);--Data   
--STM32_SR_Latch (2) <= SR_M2_IN;
--STM32_SR_Latch (1) <= SR_M1_IN;
--STM32_SR_Latch (0) <= SR_M0_IN;


u_ibuf_bclk : IBUF port map(I => Clock_SI, O => bclk_ibuf);

u_bufg_bclk : BUFG port map(I => bclk_ibuf, O => si_bclk);

u_ibuf_stmclk : IBUF port map(I => STM32_ACK, O => stm_ack);

u_sr : entity work.sr_latch_ctrl
  port map(
    clk_ctrl  => mhz100clk,
    rst_n     => rst_n_fpga,

    sr_stb_in => STM32_SR_STB,              -- PA6
    fam_in    => SR_FAM_IN,              -- PD0
    m_in      => STM32_SR_Latch,  -- PC14:PC13:PB2

    fam_sel   => fam_sel,
    rate_code => rate_code,
    upd_pulse => sr_upd
  );
  
  
  u_arst : entity work.audio_rst_stretch
  generic map(
    HOLD_CYCLES => 100000   -- ~1ms @98.304MHz (istersen düþür)
  )
  port map(
    clk_ctrl  => mhz100clk,
    rst_n_in  => rst_n_fpga,
    kick      => sr_upd,
    rst_n_out => rst_n_audio
  );



bitrt: bitrate 
	port map
	(
		CLK	 => FCLK,
		D	 => i2s_out(2),
		Q    => shift_out_bitrate
	);
	
process(mhz12clk) -- Sisteminizdeki en uygun sürekli clock'u kullanýn
begin
    if rising_edge(mhz12clk) then
          SPDIF_XMOS_SEL_sync1 <= SPDIF_XMOS_SEL; -- 2-stage synchronizer
		  SPDIF_XMOS_SEL_sync2 <= SPDIF_XMOS_SEL_sync1;
		  SPDIF_XMOS_SEL_sync <= SPDIF_XMOS_SEL_sync2;
    end if;
end process;

reset: entity work.global_reset
    generic map (
        CLK_FREQ    => 50_000_000,  -- 50 MHz varsayýlan clock
        RESET_TIME_MS  =>200       -- 200 ms reset süresi
    )
    port map(
        clk       => mhz50clk,
        power_on  => '1',  -- Güç açýldýðýnda '1' olan sinyal
        reset_out => reset_sig   -- Global reset çýkýþý (aktif yüksek)
    );
  
 stm32_bus: entity work.stm_parallel_to_i2s_extbclk
  port map (
    rst_n      => rst_n_audio,
    sys_clk    => mhz100clk,

    stm_ack    => stm_ack,         -- strobe
    stm_data   => STM32_DATA,      -- PD8..PD15
    stm_valid  => STM32_VALID,     -- frame boyunca 1 (senin VALID)
    stm_ready  => STM32_REQ,
    stm_en_stream  => STM32_EN,

    bclk_in    => si_bclk,
   
    i2s_bclk   => stm32_in(0),
    i2s_lrclk  => stm32_in(1),
    i2s_sd     => stm32_in(2),

    dbg_stb    => open,
    dbg_cnt  => open
  );

  
  
 
     
reclk: entity work.i2s_reclock
    port map (
      
    i2s_in_bclk  =>  RCLK_IN_SCK,
    i2s_in_lrclk =>  RCLK_IN_LRCK,
    i2s_in_sdata =>  RCLK_IN_DATA,
     
    clk_24mhz    => si_bclk,
    clk_22mhz    => si_bclk,
	 
    sampling_rate => SamplingFRQ,	 
     
    i2s_out_bclk  =>RCLK_OUT_SCK,
    i2s_out_lrclk =>RCLK_OUT_LRCK,
    i2s_out_sdata =>RCLK_OUT_DATA,
	 
    reclk_enb     =>RCLK_ON,

    rst => reset_sig
    );   
        
       
--  rclk_inst: entity work.rclk
--  port map (
        -- I2S giriþ
--        i2s_bclk_in  => i2s_out(0),
--        i2s_lrclk_in => i2s_out(1),
--        i2s_data_in  => i2s_out(2),

        -- SI5340 clock
--        si5340_clk    => Clock_SI,
--        rst           => reset_sig,
--        reclk_enb     => RCLK_ON, 

        -- I2S çýkýþ
 --       i2s_bclk_out  => RCLK_OUT_SCK,
 --       i2s_lrclk_out => RCLK_OUT_LRCK,
 --       i2s_data_out  => RCLK_OUT_DATA
 --   );
     

DSP_FILT: entity work.DSP 
    port map(
	     rst           => reset_sig, 
	     Enable        => DSP_ENB, 
	     SampFREQ      => SamplingFRQ,
		 Clk           => LRCLK,
		 Bit_Clk_in    => FCLK,--i2s_out(0),
	     LR_Clk_in     => LRCLK,--i2s_out(1),
         LR_Clk_out    => DSP_LR_Clk,		  
         SignalIn      => i2s_out(2),                  
         SignalOut     => DSP_OUT--dsp_raw_out--DSP_OUT                      
	 );   
		  
shift_o: shift_reg
	port map 
	(
		CLK	=>   FCLK,
		D	=>	 i2s_out(2),
		Q	=>   shift_out
	);

mypll: pll 
  port map (
  clk_in1 => Clock_SI_50MHZ,  
  clk_out2 => mhz12clk,
  clk_out3 => mhz50clk,
  clk_out1 => mhz100clk,   
  clk_out4 => mhz200clk   
  );
  
  sampling: clkDiv
    port map (
        clk_div => samp_div_val,
		clk_in => DATA,
        reset  => '0', 
        clk_out => LED
    );	
    
sample: sample_dedect
port map
 (
  i_clk_ref   => mhz50clk,         
  i_clk_test  => i2s_out(0),      
  i_rstb   => RESET_SAMP,          
  o_clock_freq => open, 
  o_clock_freq_int => Sampling_val    
  );
   
  
 spdifin: entity work.aes3rx
   generic map (
      -- Registers width, determines minimal baud speed of input AES3 at given master clock frequency
      reg_width =>8
		)
	port map
	(
	  clk    => mhz200clk,		
      aes3   => COAX_IN_FPGA,     
      reset  => reset_sig, 
      i_clock_freq_int => Sampling_val, 	
      sdata  => SDATA,     
      sclk   => SCLK,      
      bsync  => open,     
      lrck   => SLRCLK,      
      active => COAX_LOCK
		
	);
	
opt_in_1: entity work.aes3rx
  generic map (
      -- Registers width, determines minimal baud speed of input AES3 at given master clock frequency
      reg_width =>6
		)
	port map
	(
		clk    => mhz200clk,		
      aes3   => OPTICAL_IN_1,      
      reset  => reset_sig,  
      i_clock_freq_int => Sampling_val, 		
      sdata  => SDATA_OPT_1,     
      sclk   => SCLK_OPT_1,      
      bsync  => open,     
      lrck   => SLRCLK_OPT_1,      
      active => OPTIC_LOCK_1
		
	);	
	
opt_in_2: entity work.aes3rx
  generic map (
      -- Registers width, determines minimal baud speed of input AES3 at given master clock frequency
      reg_width =>6
		)
	port map
	(
	  clk    => mhz200clk,		
      aes3   => OPTICAL_IN_2,      
      reset  => reset_sig,  
      i_clock_freq_int => Sampling_val, 		
      sdata  => SDATA_OPT_2,     
      sclk   => SCLK_OPT_2,      
      bsync  => open,     
      lrck   => SLRCLK_OPT_2,      
      active => OPTIC_LOCK_2
		
	);	

	
bitratededect: process(FCLK)
begin
    if rising_edge(FCLK) then
        -- Timeout sayacý
        if bitrate_timeout_counter < BITRATE_TIMEOUT then
            bitrate_timeout_counter <= bitrate_timeout_counter + 1;
        else
            -- Timeout oldu, bitrate geçersiz
            stable_bitrate_L <= '0';
            stable_bitrate_R <= '0';
            valid_bit_counter_L <= 0;
            valid_bit_counter_R <= 0;
        end if;

        if (LRCLK = '1') then
            -- Sol kanal bitrate kontrolü
            shift_out_bitrate_bufL <= shift_out_bitrate;
            
            -- 14-8 arasý bitlerin tamamý 0 mý kontrolü
            if (shift_out_bitrate_bufR(11 downto 4) = "00000000") then
                if valid_bit_counter_R > 0 then
                    valid_bit_counter_R <= valid_bit_counter_R - 1;
                end if;
            else
                -- Geçerli bit tespit edildi, timeout'u sýfýrla
                bitrate_timeout_counter <= 0;
                
                if valid_bit_counter_R < VALID_THRESHOLD then
                    valid_bit_counter_R <= valid_bit_counter_R + 1;
                else
                    stable_bitrate_R <= '1';
                end if;
            end if;
            
                      
        else
            -- Sað kanal bitrate kontrolü
            shift_out_bitrate_bufR <= shift_out_bitrate;
            
            -- 14-8 arasý bitlerin tamamý 0 mý kontrolü
            if (shift_out_bitrate_bufL(11 downto 4) = "00000000") then
                if valid_bit_counter_L > 0 then
                    valid_bit_counter_L <= valid_bit_counter_L - 1;
                end if;
            else
                -- Geçerli bit tespit edildi, timeout'u sýfýrla
                bitrate_timeout_counter <= 0;
                
                if valid_bit_counter_L < VALID_THRESHOLD then
                    valid_bit_counter_L <= valid_bit_counter_L + 1;
                else
                    stable_bitrate_L <= '1';
                end if;
            end if;
            
          end if;
        
        BR_MCU <= stable_bitrate_R or stable_bitrate_L;
        LED3 <= stable_bitrate_L or stable_bitrate_R;
    end if;
end process;	

sampling_rate: process (SPDIF_XMOS_SEL_sync, sampling_val, XMOS_F0, XMOS_F1, XMOS_F2, XMOS_F3,XMOS_DSDON, F_XMOS)
				 
	begin	
	   XMOS_F0_MCU	   <= '0';
       XMOS_F1_MCU 	<= '0';
       XMOS_F2_MCU 	<= '0';
	   XMOS_F3_MCU 	<= '0';
       --XMOS_DSDON_MCU <= '1';
		SamplingFRQ <=0;
		DSP_ENB <= '0';
		F_XMOS<="0000";
		RESET_SAMP<='0';
		
    if (SPDIF_XMOS_SEL_sync = "000") then
	 
	    F_XMOS (0) <= XMOS_F0;
		F_XMOS (1) <= XMOS_F1;
		F_XMOS (2) <= XMOS_F2;
		F_XMOS (3) <= XMOS_F3;

        XMOS_F0_MCU	   <= XMOS_F0;
        XMOS_F1_MCU 	<= XMOS_F1;
        XMOS_F2_MCU 	<= XMOS_F2;
		XMOS_F3_MCU 	<= XMOS_F3;
     --   XMOS_DSDON_MCU <= XMOS_DSDON;
		RESET_SAMP<='0';
		if XMOS_DSDON = '0' then		
		  if F_XMOS = "0001" then
		  --44.1 khz
		   DSP_ENB <= '1';
		   SamplingFRQ <=0;		  
		  elsif F_XMOS = "0010" then
		  --48 khz
		   DSP_ENB <= '1';
		   SamplingFRQ <=1;
		  elsif F_XMOS = "0011" then
		  --88.2 khz
		   DSP_ENB <= '1';
		   SamplingFRQ <=2;
		  elsif F_XMOS = "0100" then
		  --96 khz
		   DSP_ENB <= '1';
		   SamplingFRQ <=3;
		  elsif F_XMOS = "0101" then
		  --176.4 khz
		   DSP_ENB <= '1';
		   SamplingFRQ <=4;		  
		  elsif F_XMOS = "0110" then
		  --196 khz
		   DSP_ENB <= '1';
		   SamplingFRQ <=5;		  
		  elsif F_XMOS = "0111" then
		  --352.8 khz
		   DSP_ENB <= '1';
		   SamplingFRQ <=6;
		  elsif F_XMOS = "1000" then
		  --384 khz
		   DSP_ENB <= '1';
		   SamplingFRQ <=7;
		  end if;		
		 else
		   RESET_SAMP<='0';
		 end if;	
		 
	else  
	  
	   RESET_SAMP<=  '1';
		
	 if (sampling_val< 210 or sampling_val >2400 ) then  --no signal
	     XMOS_F0_MCU	<= '0';
         XMOS_F1_MCU   <= '0';
         XMOS_F2_MCU   <= '0';
		 XMOS_F3_MCU   <= '0';
		 DSP_ENB <= '0';
		 SamplingFRQ <=8;		 
		 
	 elsif (sampling_val >220 and sampling_val <240) then --44.100
	 
	     XMOS_F0_MCU	<= '1';
         XMOS_F1_MCU   <= '0';
         XMOS_F2_MCU   <= '0';
		 XMOS_F3_MCU   <= '0';
		 DSP_ENB <= '1';
		 SamplingFRQ <=0;
		 
	 elsif (sampling_val <450 and sampling_val >245) then --48.000
	 
	     XMOS_F0_MCU	<= '0';
         XMOS_F1_MCU   <= '1';
         XMOS_F2_MCU   <= '0';
		 XMOS_F3_MCU   <= '0';
		 DSP_ENB <= '1';
		 SamplingFRQ   <= 1;
		 
	 elsif (sampling_val <500 and sampling_val >452) then --88.200
	 
	     XMOS_F0_MCU   <= '1';
         XMOS_F1_MCU   <= '1';
         XMOS_F2_MCU   <= '0';
		 XMOS_F3_MCU   <= '0';
		 DSP_ENB <= '1';
		 SamplingFRQ   <= 2;
		 
	 elsif (sampling_val <724 and sampling_val >501) then --96.000
	 
	     XMOS_F0_MCU	<= '0';
         XMOS_F1_MCU   <= '0';
         XMOS_F2_MCU   <= '1';
		 XMOS_F3_MCU   <= '0';
		 DSP_ENB <= '1';
		 SamplingFRQ   <= 3; 
		 
	 elsif (sampling_val <1000 and sampling_val >725) then --176.400
	 
	     XMOS_F0_MCU   <= '1';
         XMOS_F1_MCU   <= '0';
         XMOS_F2_MCU   <= '1';
		 XMOS_F3_MCU   <= '0';
		 DSP_ENB <= '1';
		 SamplingFRQ   <= 4; 
		 
	 elsif (sampling_val <1024 and sampling_val >1001) then --192.000
	 
	     XMOS_F0_MCU	<= '0';
         XMOS_F1_MCU   <= '1';
         XMOS_F2_MCU   <= '1';
		 XMOS_F3_MCU   <= '0';
		 DSP_ENB <= '1';
		 SamplingFRQ   <= 5;
	 elsif (sampling_val <1900 and sampling_val >1700) then --352.800
	 
	     XMOS_F0_MCU	<= '1';
         XMOS_F1_MCU   <= '1';
         XMOS_F2_MCU   <= '1';
		 XMOS_F3_MCU   <= '0';
		 DSP_ENB <= '1';
		 SamplingFRQ   <= 6; 
		 
	 elsif (sampling_val <2100 and sampling_val >1950) then --384.800
	 
	     XMOS_F0_MCU	<= '0';
         XMOS_F1_MCU   <= '0';
         XMOS_F2_MCU   <= '0';
		 XMOS_F3_MCU   <= '1';
		 DSP_ENB <= '1';
		 SamplingFRQ   <= 7;  
	else                                      --no signal
     	 XMOS_F0_MCU	<= '0';
         XMOS_F1_MCU   <= '0';
         XMOS_F2_MCU   <= '0';
		 XMOS_F3_MCU   <= '0';
		 DSP_ENB <= '0';
		 SamplingFRQ <=8;
		 
		end if;
	end if;

end process;


DSP_active <= '1' when (DSP_SEL='1' and SamplingFRQ < 8) else '0';
		
	
        i2s_out <= xmos_in when SPDIF_XMOS_SEL_sync="000" else
		             spdif_in when SPDIF_XMOS_SEL_sync="001" else        
		             coax_in_spdif when SPDIF_XMOS_SEL_sync="010" else 
		             opt_1_in_spdif when SPDIF_XMOS_SEL_sync="011" else 
				     opt_2_in_spdif when SPDIF_XMOS_SEL_sync="100" else
				     stm32_in when SPDIF_XMOS_SEL_sync="101" else
					 hdmi_in when SPDIF_XMOS_SEL_sync="110" else "000";			 
						 
		 
				DSD_EN  <= '0' when (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="000") else '1';
                DSD_CLK <= XMOS_FCLK when (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="000") else '0';
				DSD_A   <= (XMOS_LRCLK xor '1') when (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="000") else '0';
				DSD_B   <= (XMOS_LRCLK xor '0') when (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="000") else '0';
				DSD_A_N <= (XMOS_DATA xor '1') when (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="000") else '0';
				DSD_B_N <= (XMOS_DATA xor '0') when (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="000") else '0';
	            FCLK    <= '0' when (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="000") else i2s_out(0);
				LRCLK   <= '0' when (XMOS_DSDON='1' and SPDIF_XMOS_SEL_sync="000") else i2s_out(1);      
	       
 
  --DATA<= DSP_OUT when (DSP_active = '1') else RCLK_OUT_DATA;--i2s_out(2);
 --DATA<=  i2s_out(2);
   RCLK_IN_DATA<= DSP_OUT when (DSP_active = '1') else i2s_out(2);
   RCLK_IN_SCK <=  FCLK;
   RCLK_IN_LRCK<= DSP_LR_Clk when (DSP_active = '1') else LRCLK; 
     
	   --L_BCLK    <= not FCLK;    
      -- R_BCLK    <= not FCLK;
        DATA<= RCLK_OUT_DATA;
	   L_BCLK    <=   not RCLK_OUT_SCK;    
       R_BCLK    <=   not RCLK_OUT_SCK;
              	

 bclk_i <= RCLK_OUT_SCK;
        ws_i <= RCLK_OUT_LRCK;
       data_i <=RCLK_OUT_DATA ;
              	
sign_magnitude: process(bclk_i,  ws_i , data_i)

begin
         L_DATA_P <= DATA_L;
         L_DATA_N <= DATA_L;
       
         R_DATA_P <= DATA_R;
         R_DATA_N <= DATA_R;
		
	     L_CLK    <= RCLK_L;
	     R_CLK    <= RCLK_R;
  WSD<= ws_i;
  if rising_edge(bclk_i) then
    --SRCLK <= '0';
    RCLK_L <= '0';
    RCLK_R <= '0';

    -- LRCLK kenar deðiþimi › kanal sýfýrlama
    if WSD/= current_ch then
      current_ch <= WSD;
      bit_cnt <= 0;
    else
      if bit_cnt < 25 then
       -- Sign bit yakalama (bit 0)
        if bit_cnt = 0 then
          if current_ch = '0' then
            DATA_L <= data_i;
          else
            DATA_R <= data_i;
          end if;
        -- Geri kalan 23 bit (bit 1-23)
        else
          if current_ch = '0' then           
              DATA_L <= data_i;
          else
              DATA_R <= data_i;
          end if;
        end if;

        -- RCLK üretimi
        if bit_cnt = 24 then
          if current_ch = '0' then
            RCLK_L <= '1';
          else
            RCLK_R <= '1';
          end if;
        end if;

        bit_cnt <= bit_cnt + 1; 
		 
      end if;
    end if;
  end if;
end process;
  
end Behavioral;
