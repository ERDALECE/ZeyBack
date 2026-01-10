

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
USE ieee.numeric_std.ALL; 
use ieee.math_real.all;
 
entity DSP is
         
    port(
	     rst          : in std_logic;
	     Enable       : in std_logic;
	     SampFREQ     : integer := 0;
		 Clk          : in std_logic;
		 Bit_Clk_in   : in std_logic;
		 LR_Clk_in    : in std_logic;
         LR_Clk_out   : out std_logic;		  
         SignalIn     : in std_logic;                       
         SignalOut    : out std_logic                      
		  ); 		  
                                                   
end DSP;
 
architecture Behavioral of DSP is

signal SignalOutFilt              : STD_LOGIC;

signal FILT_Signal_in_L_44100     : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_L_44100    : SIGNED (20 DOWNTO 0);
signal FILT_Signal_in_R_44100     : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_R_44100    : SIGNED (20 DOWNTO 0);

signal FILT_Signal_in_L_48000     : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_L_48000    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_in_R_48000     : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_R_48000    : SIGNED (23 DOWNTO 0);

signal FILT_Signal_in_L_88200     : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_L_88200    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_in_R_88200     : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_R_88200    : SIGNED (23 DOWNTO 0);

signal FILT_Signal_in_L_96000     : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_L_96000    : SIGNED (20 DOWNTO 0);
signal FILT_Signal_in_R_96000     : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_R_96000    : SIGNED (20 DOWNTO 0);

signal FILT_Signal_in_L_176400    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_L_176400   : SIGNED (23 DOWNTO 0);
signal FILT_Signal_in_R_176400    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_R_176400   : SIGNED (23 DOWNTO 0);

signal FILT_Signal_in_L_192000    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_L_192000   : SIGNED (20 DOWNTO 0);
signal FILT_Signal_in_R_192000    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_R_192000   : SIGNED (20 DOWNTO 0);

signal FILT_Signal_in_L_352800    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_L_352800   : SIGNED (23 DOWNTO 0);
signal FILT_Signal_in_R_352800    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_R_352800   : SIGNED (23 DOWNTO 0);

signal FILT_Signal_in_L_384000    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_L_384000   : SIGNED (23 DOWNTO 0);
signal FILT_Signal_in_R_384000    : SIGNED (23 DOWNTO 0);
signal FILT_Signal_out_R_384000   : SIGNED (23 DOWNTO 0);

signal I2S_in_L                   : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal I2S_in_R                   : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal I2S_out_L                  : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal I2S_out_R                  : STD_LOGIC_VECTOR (31 DOWNTO 0);

signal I2S_out_L_BUF              : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal I2S_out_R_BUF              : STD_LOGIC_VECTOR (31 DOWNTO 0);

signal I2S_out_L_BUF1             : STD_LOGIC_VECTOR (31 DOWNTO 0);
signal I2S_out_R_BUF1             : STD_LOGIC_VECTOR (31 DOWNTO 0);

signal shift_out                  : STD_LOGIC_VECTOR (8 DOWNTO 0);
signal LR_CLK_OUT_BUF             : STD_LOGIC;
signal LR_CLK_OUT_BUF1            : STD_LOGIC;

signal Signal_valid_L_44100       : STD_LOGIC;
signal Signal_valid_R_44100       : STD_LOGIC;

signal Signal_valid_L_48000       : STD_LOGIC;
signal Signal_valid_R_48000       : STD_LOGIC;

signal Signal_valid_L_88200       : STD_LOGIC;
signal Signal_valid_R_88200       : STD_LOGIC;

signal Signal_valid_L_96000       : STD_LOGIC;
signal Signal_valid_R_96000       : STD_LOGIC;

signal Signal_valid_L_176400      : STD_LOGIC;
signal Signal_valid_R_176400      : STD_LOGIC;

signal Signal_valid_L_192000      : STD_LOGIC;
signal Signal_valid_R_192000      : STD_LOGIC;

signal Signal_valid_L_352800      : STD_LOGIC;
signal Signal_valid_R_352800      : STD_LOGIC;

signal Signal_valid_L_384000      : STD_LOGIC;
signal Signal_valid_R_384000      : STD_LOGIC;

component shift_reg
    Port ( D   : in  STD_LOGIC;
           CLK : in  STD_LOGIC;
           Q : out STD_LOGIC_VECTOR (8 downto 0));
end component;


component i2s_in  
-- width: How many bits (from MSB) are gathered from the serial I2S input
generic(width : integer := 24);

port(
	LR_CLK    : in  std_logic;      --Left/Right indicator clock
	BIT_CLK   : in  std_logic;      --Bit clock
	DIN       : in  std_logic;      --Data Input
	
	RESET_I     : in  std_logic;      --Asynchronous Reset (Active Low)
	
	DATA_L    : out std_logic_vector(width-1 downto 0);
	DATA_R    : out std_logic_vector(width-1 downto 0);
	
	DATA_RDY_L    : out std_logic;     --Falling edge means data is ready
	DATA_RDY_R    : out std_logic      --Falling edge means data is ready
);
end component;


component i2s_out
-- width: How many bits (from MSB) are gathered from the serial I2S input
generic(width : integer := 24);

port(
	
	LR_CLK    : in  std_logic;      --Left/Right indicator clock
	BIT_CLK   : in  std_logic;      --Bit clock
	DOUT      : out std_logic;      --Data Output
	
	RESET_I     : in  std_logic;      --Asynchronous Reset (Active Low)
	
	DATA_L    : in std_logic_vector(0 to width-1);
	DATA_R    : in std_logic_vector(0 to width-1);
	
	DATA_RDY_L    : out std_logic;      --Falling edge means data is ready
	DATA_RDY_R    : out std_logic       --Falling edge means data is ready
);
end component;


component FIRFilter
    generic(SamplingFrequency : real := 50000000.0;            
        Frequency1 : real := 10000000.0;                   
        Frequency2 : real := 20000000.0;                   
        FilterOrder : natural := 80;                       
        FilterType : string := "Lowpass";                  
        WindowType : string := "Hamming";                  
        CoefficientWidth : natural := 16;                  
        SignalWidth : natural := 8);                       
         
    port(Clk, Enable : in std_logic; 
	     SignalIn : in signed(SignalWidth - 1 downto 0);    
        SignalOut : out signed(SignalWidth - 1 downto 0);
		  ValidOut  : out std_logic); 
end component;

begin

shift_24bit: shift_reg
	port map 
	(
		CLK	=>  Bit_Clk_in,
		D	=>	SignalOutFilt,
		Q	=> shift_out
	);

IN_I2S: i2s_in 
   generic map (
     width => 32)

   port map(
	  LR_CLK    => LR_Clk_in,     
	  BIT_CLK   => Bit_Clk_in,     
	  DIN       => SignalIn,     
	
	  RESET_I     => not rst,      
	
	  DATA_L    => I2S_in_L,
	  DATA_R    => I2S_in_R,
	
	  DATA_RDY_L    => open,
	  DATA_RDY_R    => open
);

OUT_I2S: i2s_out
   generic map(
      width => 32)

   port map(
	
	  LR_CLK    => LR_CLK_OUT_BUF,   
	  BIT_CLK   => Bit_Clk_in,        
	  DOUT      => SignalOutFilt,
	  
	  RESET_I     => not rst,
	  
	  DATA_L    => I2S_out_R_BUF1,
	  DATA_R    => I2S_out_L_BUF1,
	
	  DATA_RDY_L  => open,     
	  DATA_RDY_R  => open        
);


-------- LEFT CHANNEL FIR FILTER -----------------
FIRFILT_44100_L: FIRFilter
    generic map(
	     SamplingFrequency => 44100.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 160,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 12,                 
        SignalWidth => 21)                      
         
    port map(
	      Clk =>Clk ,
			Enable => '1',                        
         SignalIn  =>signed (I2S_in_L(31 downto 11)),    
         SignalOut =>FILT_Signal_out_L_44100,
		   ValidOut => Signal_valid_L_44100		
			);
			
FIRFILT_48000_L: FIRFilter
    generic map(
	     SamplingFrequency => 48000.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 120,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 9,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk =>Clk,
			Enable => '1',                        
         SignalIn  =>signed (I2S_in_L(31 downto 8)),    
         SignalOut =>FILT_Signal_out_L_48000,
		   ValidOut => Signal_valid_L_48000	
			);	
	
FIRFILT_88200_L: FIRFilter
    generic map(
	     SamplingFrequency => 88200.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 120,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 9,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk =>Clk ,
			Enable => '1',                        
         SignalIn  =>signed (I2S_in_L(31 downto 8)),    
         SignalOut =>FILT_Signal_out_L_88200,
		   ValidOut => Signal_valid_L_88200	
			);				
			
FIRFILT_96000_L: FIRFilter
    generic map(
	     SamplingFrequency => 96000.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 120,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 12,                 
        SignalWidth => 21)                      
         
    port map(
	      Clk =>Clk ,
			Enable => '1',                        
         SignalIn  =>signed (I2S_in_L(31 downto 11)),    
         SignalOut =>FILT_Signal_out_L_96000,
		   ValidOut => Signal_valid_L_96000	
			);		
		
	FIRFILT_176400_L: FIRFilter
    generic map(
	     SamplingFrequency => 176400.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 100,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 8,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk =>Clk ,
			Enable => '1',                        
         SignalIn  =>signed (I2S_in_L(31 downto 8)),    
         SignalOut =>FILT_Signal_out_L_176400,
		   ValidOut => Signal_valid_L_176400	
			);	

FIRFILT_192000_L: FIRFilter
    generic map(
	     SamplingFrequency => 192000.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 120,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 12,                 
        SignalWidth => 21)                      
         
    port map(
	      Clk =>Clk ,
			Enable => '1',                        
         SignalIn  => signed (I2S_in_L(31 downto 11)),    
         SignalOut =>FILT_Signal_out_L_192000,
		   ValidOut => Signal_valid_L_192000	
			);	
			
			
FIRFILT_352800_L: FIRFilter
    generic map(
	     SamplingFrequency => 352800.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 100,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 8,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk =>Clk ,
			Enable => '1',                        
         SignalIn  =>signed (I2S_in_L(31 downto 8)),    
         SignalOut =>FILT_Signal_out_L_352800,
		   ValidOut => Signal_valid_L_352800	
			);								

FIRFILT_384000_L: FIRFilter
    generic map(
	     SamplingFrequency => 384000.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 100,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 8,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk =>Clk ,
			Enable => '1',                        
         SignalIn  =>signed (I2S_in_L(31 downto 8)),    
         SignalOut =>FILT_Signal_out_L_384000,
		   ValidOut => Signal_valid_L_384000	
			);						
			

----------- FIR FILTER RIGHT CHANNEL --------------------		
FIRFILT_44100_R: FIRFilter
    generic map(
	     SamplingFrequency => 44100.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 160,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 12,                 
        SignalWidth => 21)                      
         
    port map(
	      Clk => Clk ,
			Enable => '1',                      
         SignalIn =>signed (I2S_in_R(31 downto 11)),    
         SignalOut =>FILT_Signal_out_R_44100,
		   ValidOut => Signal_valid_R_44100	
			);
			
			
FIRFILT_48000_R: FIRFilter
    generic map(
	     SamplingFrequency => 48000.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 120,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 9,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk => Clk ,
			Enable => '1',                      
         SignalIn =>signed (I2S_in_R(31 downto 8)),    
         SignalOut =>FILT_Signal_out_R_48000,
		   ValidOut => Signal_valid_R_48000	
			);		
		
	
FIRFILT_88200_R: FIRFilter
    generic map(
	     SamplingFrequency => 88200.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 120,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 9,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk => Clk ,
			Enable => '1',                      
         SignalIn =>signed (I2S_in_R(31 downto 8)),    
         SignalOut =>FILT_Signal_out_R_88200,
		   ValidOut => Signal_valid_R_88200	
			);	
			
FIRFILT_96000_R: FIRFilter
    generic map(
	     SamplingFrequency => 96000.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 120,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 12,                 
        SignalWidth => 21)                      
         
    port map(
	      Clk => Clk ,
			Enable => '1',                      
         SignalIn =>signed (I2S_in_R(31 downto 11)),    
         SignalOut =>FILT_Signal_out_R_96000,
		   ValidOut => Signal_valid_R_96000	
			);	
		
	
FIRFILT_176400_R: FIRFilter
    generic map(
	     SamplingFrequency => 176400.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 100,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 8,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk => Clk ,
			Enable => '1',                      
         SignalIn =>signed (I2S_in_R(31 downto 8)),    
         SignalOut =>FILT_Signal_out_R_176400,
		   ValidOut => Signal_valid_R_176400	
			);	
			
FIRFILT_192000_R: FIRFilter
    generic map(
	     SamplingFrequency => 192000.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 120,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 12,                 
        SignalWidth => 21)                      
         
    port map(
	      Clk => Clk ,
			Enable => '1',                      
         SignalIn => signed (I2S_in_R(31 downto 11)),    
         SignalOut =>FILT_Signal_out_R_192000,
		   ValidOut => Signal_valid_R_192000	
			);
			
FIRFILT_352800_R: FIRFilter
    generic map(
	     SamplingFrequency => 352800.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 100,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 8,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk => Clk ,
			Enable => '1',                      
         SignalIn =>signed (I2S_in_R(31 downto 8)),    
         SignalOut =>FILT_Signal_out_R_352800,
		   ValidOut => Signal_valid_R_352800	
			);					
			
FIRFILT_384000_R: FIRFilter
    generic map(
	     SamplingFrequency => 384000.0,            
        Frequency1 => 20000.0,                   
        Frequency2 => 20000.0,                  
        FilterOrder => 100,                       
        FilterType => "Lowpass",                  
        WindowType => "Hamming",                  
        CoefficientWidth => 8,                 
        SignalWidth => 24)                      
         
    port map(
	      Clk => Clk ,
			Enable => '1',                      
         SignalIn =>signed (I2S_in_R(31 downto 8)),    
         SignalOut =>FILT_Signal_out_R_384000,
		   ValidOut => Signal_valid_R_384000	
			);			
			
---------------------------------------------------------------			
	--SignalOut<= shift_out(7);
SignalOut<= SignalOutFilt;	

dsp_filt: process (Bit_Clk_in, SampFREQ, I2S_in_L, I2S_in_R)
VARIABLE ws_cnt: INTEGER := 0;  
 begin  
  if rising_edge (Bit_Clk_in) then	 
    if(ws_cnt <31) THEN 
	     ws_cnt := ws_cnt + 1; 
	   if (LR_CLK_OUT_BUF ='0' ) then		
       if SampFREQ = 0 then 
		  if Signal_valid_L_44100 = '1' then
	     I2S_out_L (31 downto 11)  <= std_logic_vector (FILT_Signal_out_L_44100);
		  else
		  I2S_out_L (31 downto 11)  <= (others => '0');
		  end if;
		  I2S_out_L (10 downto 0) <= (others=>'0');
		 elsif SampFREQ = 1 then
		   if Signal_valid_L_48000 = '1' then
	   	I2S_out_L (31 downto 8)  <= std_logic_vector (FILT_Signal_out_L_48000);
			else
			I2S_out_L (31 downto 8)  <= (others => '0');
			end if;
			I2S_out_L (7 downto 0) <= (others=>'0');
		 elsif SampFREQ = 2 then
		    if Signal_valid_L_88200 = '1' then 
	   	I2S_out_L (31 downto 8)  <= std_logic_vector (FILT_Signal_out_L_88200);
			else
			I2S_out_L (31 downto 8)  <= (others => '0');
			end if;
			I2S_out_L (7 downto 0) <= (others=>'0');
		 elsif SampFREQ = 3 then 
		   if Signal_valid_L_96000 = '1' then
	   	I2S_out_L (31 downto 11)  <= std_logic_vector (FILT_Signal_out_L_96000);
			else
			I2S_out_L (31 downto 11)  <= (others => '0');
			end if;
			I2S_out_L (10 downto 0) <= (others=>'0');
		 elsif SampFREQ = 4 then 
		   if Signal_valid_L_176400 = '1' then
	   	I2S_out_L (31 downto 8)  <= std_logic_vector (FILT_Signal_out_L_176400);
			else
			I2S_out_L (31 downto 8)  <= (others => '0');
			end if;
			I2S_out_L (7 downto 0) <= (others=>'0');
		 elsif SampFREQ = 5 then 
		    if Signal_valid_L_192000 = '1' then
	   	I2S_out_L (31 downto 11)  <= std_logic_vector (FILT_Signal_out_L_192000);
			 else
			 I2S_out_L (31 downto 11)  <= (others => '0');
			 end if;
			I2S_out_L (10 downto 0) <= (others=>'0');	
	    elsif SampFREQ = 6 then 
		   if Signal_valid_L_352800 = '1' then
	   	I2S_out_L (31 downto 8)  <= std_logic_vector (FILT_Signal_out_L_352800);
			else
			I2S_out_L (31 downto 8)  <= (others => '0');
			end if;
			I2S_out_L (7 downto 0) <= (others=>'0');	
		 elsif SampFREQ = 7 then 
		   if Signal_valid_L_384000 = '1' then
	   	I2S_out_L (31 downto 8)  <= std_logic_vector (FILT_Signal_out_L_384000);
			else
			I2S_out_L (31 downto 8)  <= (others => '0');
			end if;
			I2S_out_L (7 downto 0) <= (others=>'0');	
	    end if;			
	 else
		if SampFREQ = 0 then
         if Signal_valid_R_44100 = '1' then     
         I2S_out_R (31 downto 11) <= std_logic_vector (FILT_Signal_out_R_44100);
		   else
			I2S_out_R (31 downto 11)  <= (others => '0');
			end if;
		  I2S_out_R (10 downto 0) <= (others=>'0');
	   elsif SampFREQ = 1 then 
		  if Signal_valid_R_48000 = '1' then     
		  I2S_out_R (31 downto 8) <= std_logic_vector (FILT_Signal_out_R_48000);
		  else
		  I2S_out_R (31 downto 8)  <= (others => '0');
		  end if;
		  I2S_out_R (7 downto 0) <= (others=>'0');
		elsif SampFREQ = 2 then 
		  if Signal_valid_R_88200 = '1' then
		  I2S_out_R (31 downto 8) <= std_logic_vector (FILT_Signal_out_R_88200);
		  else
		  I2S_out_R (31 downto 8)  <= (others => '0');
		  end if;
		  I2S_out_R (7 downto 0) <= (others=>'0');
		elsif SampFREQ = 3 then 
		  if Signal_valid_R_96000 = '1' then
		  I2S_out_R (31 downto 11) <= std_logic_vector (FILT_Signal_out_R_96000);
		  else
		  I2S_out_R (31 downto 11)  <= (others => '0');
		  end if;
		  I2S_out_R (10 downto 0) <= (others=>'0');
		elsif SampFREQ = 4 then 
		  if Signal_valid_R_176400 = '1' then
		  I2S_out_R (31 downto 8) <= std_logic_vector (FILT_Signal_out_R_176400);
		  else
		  I2S_out_R (31 downto 8)  <= (others => '0');
		  end if;
		  I2S_out_R (7 downto 0) <= (others=>'0');
		elsif SampFREQ = 5 then 
		  if Signal_valid_R_192000 = '1' then
		  I2S_out_R (31 downto 11) <= std_logic_vector (FILT_Signal_out_R_192000);
		  else
		   I2S_out_R (31 downto 11) <= (others=>'0');
			end if;
		  I2S_out_R (10 downto 0) <= (others=>'0');
      elsif SampFREQ = 6 then 
		  if Signal_valid_R_352800 = '1' then
		  I2S_out_R (31 downto 8) <= std_logic_vector (FILT_Signal_out_R_352800);
		  else
		   I2S_out_R (31 downto 8) <= (others=>'0');
		  end if;
		  I2S_out_R (7 downto 0) <= (others=>'0');	
      elsif SampFREQ = 7 then 
		  if Signal_valid_R_384000 = '1' then
		  I2S_out_R (31 downto 8) <= std_logic_vector (FILT_Signal_out_R_384000);
		  else
		  I2S_out_R (31 downto 8) <= (others=>'0');
		  end if;
		  I2S_out_R (7 downto 0) <= (others=>'0');		 		  
		end if;
	 end if;	
 else
	   I2S_out_R_BUF <= I2S_out_R; 
		I2S_out_L_BUF <= I2S_out_L; 
		I2S_out_R_BUF1 <= I2S_out_R_BUF; 
		I2S_out_L_BUF1 <= I2S_out_L_BUF; 
		ws_cnt := 0;	  
	   LR_CLK_OUT_BUF <= NOT LR_CLK_OUT_BUF;
	   LR_Clk_out <= LR_CLK_OUT_BUF;	
   		
 end if;	 
end if;
end process;	
			
end architecture;