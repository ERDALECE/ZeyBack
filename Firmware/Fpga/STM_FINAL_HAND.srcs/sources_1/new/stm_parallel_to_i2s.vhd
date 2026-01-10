-------------------------------------------------------------------------------
-- STM32 8-bit parallel (strobe) -> Async FIFO -> I2S using EXTERNAL BCLK
--
-- Write domain : mhz50clk (RX samples strobe edges into sys_clk)
-- Read domain  : bclk_in  (from SI5340: Fs*64)
-- I2S format   : 32-bit slot per channel (64 BCLK/frame), I2S 1-bit delay
--
-- Notes:
--  * bclk_in MUST be routed as a real clock (IBUF + BUFG in top).
--  * stm_clk is NOT a clock; it is sampled/synchronized inside stm8_parallel_rx_strobe.
--  * FIFO is FWFT so fifo_dout is valid whenever empty='0'. We latch one word per frame.
---------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library xpm;
use xpm.vcomponents.all;

entity stm_parallel_to_i2s_extbclk is
  generic (
    FIFO_DEPTH : integer := 32768
  );
  port (
    rst_n      : in  std_logic;
    sys_clk   : in  std_logic;

    -- STM32 side
    stm_ack    : in  std_logic; -- strobe
    stm_data   : in  std_logic_vector(7 downto 0);
    stm_valid  : in  std_logic; -- frame enable (VALID)
    stm_ready  : out std_logic; -- backpressure to STM32
    stm_en_stream    : in  std_logic; -- strobe
    -- External BCLK from SI5340 (Fs*64)
    bclk_in    : in  std_logic;

    -- I2S out
    i2s_bclk   : out std_logic;
    i2s_lrclk  : out std_logic;
    i2s_sd     : out std_logic;

    -- debug
    dbg_stb    : out std_logic;
    dbg_cnt    : out std_logic_vector(2 downto 0)
  );
end entity;

architecture rtl of stm_parallel_to_i2s_extbclk is
  constant W : integer := 64;

  signal fifo_rst      : std_logic;
  signal fifo_din      : std_logic_vector(W-1 downto 0);
  signal fifo_wr_en    : std_logic;
  signal fifo_full     : std_logic;
  signal fifo_prog_full: std_logic;

  signal fifo_dout     : std_logic_vector(W-1 downto 0);
  signal fifo_rd_en    : std_logic;
  signal fifo_empty    : std_logic;

  -- FIFO read enable must be asserted synchronous to rd_clk rising edge (XPM FIFO requirement)
  signal fifo_rd_en_r   : std_logic := '0';
  -- Falling-edge TX toggles this request once per frame; rising-edge process turns it into 1-cycle rd_en pulse
  signal fifo_pop_tgl   : std_logic := '0';
  signal fifo_pop_sync  : std_logic := '0';


  -- RX (write domain)
  signal l_rx, r_rx : std_logic_vector(23 downto 0);
  signal stb_rx     : std_logic;

  -- Read/Tx domain registers
  signal l_act, r_act : std_logic_vector(23 downto 0) := (others => '0');
  signal shreg        : std_logic_vector(31 downto 0) := (others => '0');
  signal sd_reg       : std_logic := '0';
  signal lr_reg       : std_logic := '0';
  signal bit_in_fr    : integer range 0 to 63 := 0;
  
  -- debug counter: counts received bytes 0..5 in RX module
  signal dbg_cnt_i    : std_logic_vector(2 downto 0) := (others => '0');
   
begin
  fifo_rst <= not rst_n;

  -- Pass-through BCLK (already BUFG'ed in top)
  i2s_bclk  <= bclk_in;
  i2s_lrclk <= lr_reg;
  i2s_sd    <= sd_reg;

  dbg_stb <= stb_rx;
  dbg_cnt <= dbg_cnt_i;

  -- Debug counter in write clock domain (counts completed stereo frames)
  process(sys_clk)
  begin
    if rising_edge(sys_clk) then
      if rst_n='0' then
        dbg_cnt_i <= (others=>'0');
      else
        if stb_rx='1' then
          dbg_cnt_i <= std_logic_vector(unsigned(dbg_cnt_i) + 1);
        end if;
      end if;
    end if;
  end process;


  -- ===== RX: 8-bit strobe -> 24-bit L/R (mhz50clk domain)
 u_rx : entity work.stm8_parallel_rx_toggle_sync_req
    generic map(
      CAP_DLY         => 1,       -- sys_clk=100MHz: start with 1 (try 0/2 if needed)
      SYNC_GUARD      => 2,    -- ~200us watchdog to avoid deadlock
      VAL_WAIT1_CYC  => 2,  -- cycles after strobe-event before sample #1
      VAL_WAIT2_CYC  => 1,  -- cycles between sample #1 and #2
      VAL_WAIT3_CYC  => 1  
    
    )
    port map (
      sys_clk        => sys_clk,
      rst_n          => rst_n,
      en_stream      => stm_en_stream, -- or map to stm_valid if you want to gate streaming
      
      sync_in        => stm_valid,
      stm_tgl_in     => stm_ack,   -- ACK level from STM32 (was toggle clock)
      data_in        => stm_data,

      fpga_req_out   => stm_ready, -- REQ level to STM32 (replaces READY)
      fifo_prog_full => fifo_prog_full,

      sample_l       => l_rx,
      sample_r       => r_rx,
      sample_stb     => stb_rx
    ); 

  -- Pack into 64-bit word: [63:32]=L(24)+pad8, [31:0]=R(24)+pad8
  fifo_din   <= l_rx & x"00" & r_rx & x"00";
  fifo_wr_en <= stb_rx and (not fifo_full);
  
 

  -- ===== XPM FIFO async (FWFT)
  u_fifo : xpm_fifo_async
    generic map(
      FIFO_MEMORY_TYPE    => "block",
      FIFO_WRITE_DEPTH    => FIFO_DEPTH,
      WRITE_DATA_WIDTH    => W,
      READ_DATA_WIDTH     => W,
      READ_MODE           => "fwft",
      FIFO_READ_LATENCY   => 0,
      ECC_MODE            => "no_ecc",
      CDC_SYNC_STAGES     => 2,
      PROG_FULL_THRESH    => FIFO_DEPTH-256,
      PROG_EMPTY_THRESH   => 256
    )
    port map(
      rst        => fifo_rst,

      wr_clk     => sys_clk,
      wr_en      => fifo_wr_en,
      din        => fifo_din,
      full       => fifo_full,
      prog_full  => fifo_prog_full,
      wr_data_count => open,

      rd_clk     => bclk_in,
      rd_en      => fifo_rd_en_r,
      dout       => fifo_dout,
      empty      => fifo_empty,
      data_valid => open,
      rd_data_count => open,

      almost_empty  => open,
      almost_full   => open,
      prog_empty    => open,
      overflow      => open,
      underflow     => open,
      wr_rst_busy   => open,
      rd_rst_busy   => open,
      sleep         => '0',
      injectsbiterr => '0',
      injectdbiterr => '0',
      sbiterr       => open,
      dbiterr       => open
    );

  -- ===== FIFO pop pulse aligned to BCLK rising edge (required for XPM FIFO rd_en)
  process(bclk_in)
    variable pulse : std_logic;
  begin
    if rising_edge(bclk_in) then
      if rst_n='0' then
        fifo_pop_sync <= '0';
        fifo_rd_en_r  <= '0';
      else
        -- pulse goes high for 1 BCLK rising-edge cycle when fifo_pop_tgl changes
        pulse := fifo_pop_tgl xor fifo_pop_sync;
        fifo_pop_sync <= fifo_pop_tgl;
        fifo_rd_en_r  <= pulse;
      end if;
    end if;
  end process;

  -- ===== I2S generator on BCLK falling edge (proper I2S timing)
  process(bclk_in)
    variable next_bit_fr : integer;
    variable next_lr     : std_logic;
    variable ch_bit      : integer;
    variable load_word   : std_logic_vector(31 downto 0);
    variable l_use, r_use: std_logic_vector(23 downto 0);
  begin
    if falling_edge(bclk_in) then
      if rst_n='0' then
        lr_reg    <= '0';
        bit_in_fr <= 0;
        shreg     <= (others=>'0');
        sd_reg    <= '0';
        l_act     <= (others=>'0');
        r_act     <= (others=>'0');
        fifo_pop_tgl <= '0';
      else
        -- default: no direct pop here (rd_en is generated on BCLK rising edge via fifo_pop_tgl toggle)
        -- next bit position (0..63)
        if bit_in_fr = 63 then
          next_bit_fr := 0;
        else
          next_bit_fr := bit_in_fr + 1;
        end if;

        -- LRCLK transitions at channel boundary (based on next bit pos)
  --      if next_bit_fr = 0 then
  --        next_lr := '0';
  --      elsif next_bit_fr = 32 then
  --        next_lr := '1';
  --      else
  --        next_lr := lr_reg;
  --      end if;
  --      lr_reg <= next_lr;
        
         if next_bit_fr = 0 then
          next_lr := '0';
        elsif next_bit_fr = 32 then
          next_lr := '1';
        end if;
        lr_reg <= next_lr;

        ch_bit := next_bit_fr mod 32;

        -- At start of a new frame (next_bit_fr=0): latch next stereo word.
        -- FIFO is FWFT, so fifo_dout is already valid when empty='0'.
        l_use := l_act;
        r_use := r_act;
        if next_bit_fr = 0 then
          if fifo_empty = '0' then
            l_use := fifo_dout(63 downto 40);
            r_use := fifo_dout(31 downto 8);
            l_act <= l_use;
            r_act <= r_use;
            -- FWFT: assert rd_en so FIFO advances to next word on next BCLK rising edge
            fifo_pop_tgl <= not fifo_pop_tgl;  -- request FIFO advance on next BCLK rising edge
          else
            -- FIFO empty: hold previous l_act/r_act (zero-order hold) to avoid pop/click
            -- (no rd_en asserted)
            null;
          end if;
        end if;

        -- I2S 1-bit delay:
        -- ch_bit=0 -> SD=0, load 32-bit slot word
        -- ch_bit=1 -> output MSB
        if ch_bit = 0 then
          sd_reg <= '0';

          if next_lr = '1' then
            load_word := r_use & x"00";
          else
            load_word := l_use & x"00";
          end if;
          shreg <= load_word;

        else
          sd_reg <= shreg(31);
          shreg  <= shreg(30 downto 0) & '0';
        end if;

        bit_in_fr <= next_bit_fr;
      end if;
      
    end if;
  end process;

end architecture;

