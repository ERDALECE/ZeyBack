----------------------------------------------------------------------------------
-- FMC 32-bit -> Async FIFO -> I2S (PCM) / DSD (Native) Output
--
-- Write domain : sys_clk
-- Read domain  : bclk_in  (Fs*64 for PCM, DSD_CLK for DSD)
--
-- PCM Mode:
--   I2S format: 32-bit slot per channel (64 BCLK/frame), I2S 1-bit delay
--
-- DSD Mode:
--   Direct 1-bit DSD output per channel
--   DSD_CLK = SI5340 output (2.8224/5.6448/11.2896/22.5792 MHz)
--
-- Address map (A1:A0):
--   00 : DATA push (32-bit). First write=L, second write=R => one stereo frame.
--   01 : STATUS read (32-bit) / RESYNC write (phase reset)
--
-- NWAIT (active-low) asserts when FIFO prog_full OR stream disabled/flush.
----------------------------------------------------------------------------------
 
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
 
library xpm;
use xpm.vcomponents.all;
 
entity fmc32_to_i2s_dsd is
  generic (
    FIFO_DEPTH       : integer := 32768;
    FIFO_DEPTH_TRSH  : integer := 4096;
 
    -- Capture robustness (sys_clk cycles):
    WR_CAP_DLY       : integer := 2;
    PAIR_WD_CYC      : integer := 20000;
 
    -- true  = take sample24 = D[23:0]   (LSB-aligned 24-bit in 32-bit container)
    -- false = take sample24 = D[31:8]   (MSB-aligned 24-bit)
    SAMPLE_LSB_ALIGNED : boolean := false
  );
  port (
    rst_n      : in  std_logic;
    sys_clk    : in  std_logic;
 
    -- Stream enable from MCU (flush FIFO on rising edge)
    en_stream  : in  std_logic;
    
    -- DSD mode select from MCU (active high = DSD, low = PCM/I2S)
    dsd_mode   : in  std_logic;
 
    -- FMC (SRAM-like) interface
    fmc_ne1_n  : in  std_logic;
    fmc_noe_n  : in  std_logic;
    fmc_nwe_n  : in  std_logic;
    fmc_nwait  : out std_logic;  -- active low
    fmc_a      : in  std_logic_vector(1 downto 0);   -- A0..A1
    fmc_d      : inout std_logic_vector(31 downto 0);
 
    -- External BCLK from SI5340 (Fs*64 for PCM, DSD_CLK for DSD)
    bclk_in    : in  std_logic;
 
    -- I2S out (active in PCM mode)
    i2s_bclk   : out std_logic;
    i2s_lrclk  : out std_logic;
    i2s_sd     : out std_logic;
    
    -- DSD out (active in DSD mode)
    dsd_clk    : out std_logic;
    dsd_a      : out std_logic;   -- Left channel DSD data
    dsd_b      : out std_logic;   -- Right channel DSD data
    dsd_a_n    : out std_logic;   -- Left channel DSD data (inverted for diff)
    dsd_b_n    : out std_logic;   -- Right channel DSD data (inverted for diff)
    dsd_en     : out std_logic;   -- DSD enable (active high when DSD mode)
 
    -- debug
    dbg_stb    : out std_logic;
    dbg_cnt    : out std_logic_vector(2 downto 0)
  );
end entity;
 
architecture rtl of fmc32_to_i2s_dsd is
  constant W : integer := 64;
  
  -- DSD bits per 32-bit word
  constant DSD_BITS_PER_WORD : integer := 32;
  
 
  -- Stream start/stop hygiene:
  signal fifo_rst       : std_logic;
  signal stream_en_ff   : std_logic_vector(2 downto 0) := (others=>'0');
  signal stream_en_s    : std_logic := '0';
  signal stream_en_prev : std_logic := '0';
  signal stream_rst     : std_logic := '0';
  signal stream_rst_cnt : unsigned(7 downto 0) := (others=>'0');
  
  -- DSD mode synchronized
  signal dsd_mode_ff    : std_logic_vector(2 downto 0) := (others=>'0');
  signal dsd_mode_s     : std_logic := '0';
 
  -- stream_rst synchronized into BCLK domain
  signal stream_rst_bff : std_logic_vector(1 downto 0) := (others=>'0');
  signal stream_rst_b   : std_logic := '0';
 
  -- stream enable synchronized into BCLK domain
  signal stream_en_bff  : std_logic_vector(1 downto 0) := (others=>'0');
  signal stream_en_b    : std_logic := '0';
  
  -- DSD mode synchronized into BCLK domain
  signal dsd_mode_bff   : std_logic_vector(1 downto 0) := (others=>'0');
  signal dsd_mode_b     : std_logic := '0';
 
  -- FIFO signals
  signal fifo_din        : std_logic_vector(W-1 downto 0);
  signal fifo_wr_en      : std_logic;
  signal fifo_full       : std_logic;
  signal fifo_prog_full  : std_logic;
  signal fifo_prog_empty : std_logic;
  signal fifo_dout       : std_logic_vector(W-1 downto 0);
  signal fifo_empty      : std_logic;
  signal fifo_rd_en_r    : std_logic := '0';
  signal fifo_pop_tgl    : std_logic := '0';
  signal fifo_pop_sync   : std_logic := '0';
 
  -- RX packed samples (write domain)
  signal l_rx, r_rx   : std_logic_vector(23 downto 0) := (others=>'0');
  signal stb_rx        : std_logic := '0';
  signal dbg_cnt_i     : std_logic_vector(2 downto 0) := (others=>'0');
  signal fifo_din_r    : std_logic_vector(63 downto 0) := (others=>'0');
  signal fifo_wr_en_r  : std_logic := '0';
 
  -- TX domain regs (PCM/I2S)
  signal l_act, r_act : std_logic_vector(23 downto 0) := (others => '0');
  signal shreg        : std_logic_vector(31 downto 0) := (others => '0');
  signal sd_reg       : std_logic := '0';
  signal lr_reg       : std_logic := '0';
  signal bit_in_fr    : integer range 0 to 63 := 0;
  
  -- TX domain regs (DSD)
  signal dsd_l_shreg  : std_logic_vector(31 downto 0) := (others => '0');
  signal dsd_r_shreg  : std_logic_vector(31 downto 0) := (others => '0');
  signal dsd_bit_cnt  : integer range 0 to 31 := 0;
  signal dsd_a_reg    : std_logic := '0';
  signal dsd_b_reg    : std_logic := '0';
 
  -- FMC sync
  signal ne_ff, noe_ff, nwe_ff : std_logic_vector(1 downto 0) := (others=>'1');
  signal a_ff                  : std_logic_vector(1 downto 0) := (others=>'0');
  signal ne_s, noe_s, nwe_s    : std_logic := '1';
 
  -- write capture helper
  signal wr_act      : std_logic := '0';
  signal wr_act_d    : std_logic := '0';
  signal wr_seen     : std_logic := '0';
  signal cap_cnt     : unsigned(7 downto 0) := (others=>'0');
 
  signal lat_a       : std_logic_vector(1 downto 0) := (others=>'0');
  signal lat_d       : std_logic_vector(31 downto 0) := (others=>'0');
  signal got_word    : std_logic := '0';
 
  -- L/R pairing
  signal have_l      : std_logic := '0';
  signal l_tmp       : std_logic_vector(23 downto 0) := (others=>'0');
  signal wd_cnt      : unsigned(15 downto 0) := (others=>'0');
 
  -- FMC read data
  signal rd_act      : std_logic := '0';
  signal d_out       : std_logic_vector(31 downto 0) := (others=>'0');
  signal d_oe        : std_logic := '0';
 
  function to_u32(s : std_logic_vector) return unsigned is
  begin
    return unsigned(s);
  end function;
 
  function sel24(w : std_logic_vector(31 downto 0)) return std_logic_vector is
    variable v : std_logic_vector(23 downto 0);
  begin
    if SAMPLE_LSB_ALIGNED then
      v := w(23 downto 0);
    else
      v := w(31 downto 8);
    end if;
    return v;
  end function;
  
  -- For DSD: select 32 bits based on alignment
  function sel32_dsd(w : std_logic_vector(31 downto 0)) return std_logic_vector is
  begin
    -- DSD data comes as raw 32-bit words, no alignment needed
    return w;
  end function;
 
begin
  ---------------------------------------------------------------------------
  -- Output assignments based on mode
  ---------------------------------------------------------------------------
  -- I2S outputs (active in PCM mode)
  i2s_bclk  <= bclk_in when dsd_mode_b = '0' else '0';
  i2s_lrclk <= lr_reg  when dsd_mode_b = '0' else '0';
  i2s_sd    <= sd_reg  when dsd_mode_b = '0' else '0';
  
  -- DSD outputs (active in DSD mode)
  dsd_clk   <= bclk_in when dsd_mode_b = '1' else '0';
  dsd_a     <= dsd_a_reg when dsd_mode_b = '1' else '0';
  dsd_b     <= dsd_b_reg when dsd_mode_b = '1' else '0';
  dsd_a_n   <= not dsd_a_reg when dsd_mode_b = '1' else '1';
  dsd_b_n   <= not dsd_b_reg when dsd_mode_b = '1' else '1';
  dsd_en    <= dsd_mode_b;
 
  dbg_stb <= stb_rx;
  dbg_cnt <= dbg_cnt_i;
 
  ---------------------------------------------------------------------------
  -- Stream enable sync + DSD mode sync + stream-start flush pulse
  ---------------------------------------------------------------------------
  process(sys_clk)
  begin
    if rising_edge(sys_clk) then
      if rst_n='0' then
        stream_en_ff   <= (others=>'0');
        stream_en_s    <= '0';
        stream_en_prev <= '0';
        stream_rst     <= '0';
        stream_rst_cnt <= (others=>'0');
        dsd_mode_ff    <= (others=>'0');
        dsd_mode_s     <= '0';
      else
        -- Stream enable sync
        stream_en_ff(0) <= en_stream;
        stream_en_ff(1) <= stream_en_ff(0);
        stream_en_ff(2) <= stream_en_ff(1);
        stream_en_s     <= stream_en_ff(2);
        stream_en_prev  <= stream_en_s;
        
        -- DSD mode sync
        dsd_mode_ff(0) <= dsd_mode;
        dsd_mode_ff(1) <= dsd_mode_ff(0);
        dsd_mode_ff(2) <= dsd_mode_ff(1);
        dsd_mode_s     <= dsd_mode_ff(2);
 
        -- On rising edge: flush FIFO for a short while
        if (stream_en_prev='0' and stream_en_s='1') then
          stream_rst_cnt <= to_unsigned(64, stream_rst_cnt'length);
          stream_rst     <= '1';
        elsif (stream_rst_cnt /= 0) then
          stream_rst_cnt <= stream_rst_cnt - 1;
          stream_rst     <= '1';
        else
          stream_rst     <= '0';
        end if;
      end if;
    end if;
  end process;
 
  fifo_rst <= (not rst_n) or stream_rst;
 
  ---------------------------------------------------------------------------
  -- FMC input sync (control lines)
  ---------------------------------------------------------------------------
  process(sys_clk)
  begin
    if rising_edge(sys_clk) then
      if rst_n='0' then
        ne_ff  <= (others=>'1');
        noe_ff <= (others=>'1');
        nwe_ff <= (others=>'1');
        a_ff   <= (others=>'0');
      else
        ne_ff(0)  <= fmc_ne1_n;
        ne_ff(1)  <= ne_ff(0);
        noe_ff(0) <= fmc_noe_n;
        noe_ff(1) <= noe_ff(0);
        nwe_ff(0) <= fmc_nwe_n;
        nwe_ff(1) <= nwe_ff(0);
        a_ff <= fmc_a;
      end if;
    end if;
  end process;
 
  ne_s  <= ne_ff(1);
  noe_s <= noe_ff(1);
  nwe_s <= nwe_ff(1);
 
  wr_act <= '1' when (ne_s='0' and nwe_s='0') else '0';
  rd_act <= '1' when (ne_s='0' and noe_s='0' and nwe_s='1') else '0';
 
  ---------------------------------------------------------------------------
  -- NWAIT generation (active-low)
  ---------------------------------------------------------------------------
  fmc_nwait <= '0' when (fifo_prog_full='1') or (stream_en_s='0') or (stream_rst='1') else '1';
 
  ---------------------------------------------------------------------------
  -- FMC Data bus (bidirectional)
  ---------------------------------------------------------------------------
  fmc_d <= d_out when d_oe='1' else (others => 'Z');
  
  -- Read data: status register
  d_out <= x"000000" & 
           fifo_prog_empty & fifo_prog_full & fifo_full & fifo_empty &
           "00" & dsd_mode_s & stream_en_s;
  d_oe  <= rd_act;
 
  ---------------------------------------------------------------------------
  -- Write capture
  ---------------------------------------------------------------------------
  process(sys_clk)
  begin
    if rising_edge(sys_clk) then
      if rst_n='0' then
        wr_act_d  <= '0';
        wr_seen   <= '0';
        cap_cnt   <= (others=>'0');
        lat_a     <= (others=>'0');
        lat_d     <= (others=>'0');
        got_word  <= '0';
      else
        wr_act_d <= wr_act;
        got_word <= '0';
 
        if wr_act='1' then
          if wr_act_d='0' then
            cap_cnt <= to_unsigned(WR_CAP_DLY, cap_cnt'length);
          elsif wr_seen='0' then
            if cap_cnt = 0 then
              lat_a    <= a_ff;
              lat_d    <= fmc_d;
              got_word <= '1';
              wr_seen  <= '1';
            else
              cap_cnt <= cap_cnt - 1;
            end if;
          end if;
        else
          wr_seen <= '0';
        end if;
      end if;
    end if;
  end process;
 
  ---------------------------------------------------------------------------
  -- Pairing logic: first write=L, second write=R -> commit stereo frame to FIFO
  -- DSD mode: 32-bit L, 32-bit R = 64 DSD bits total
  -- PCM mode: 24-bit L, 24-bit R (packed with padding)
  ---------------------------------------------------------------------------
  process(sys_clk)
    variable s24 : std_logic_vector(23 downto 0);
    variable s32 : std_logic_vector(31 downto 0);
  begin
    if rising_edge(sys_clk) then
      if rst_n='0' then
        have_l       <= '0';
        l_tmp        <= (others=>'0');
        l_rx         <= (others=>'0');
        r_rx         <= (others=>'0');
        stb_rx       <= '0';
        wd_cnt       <= (others=>'0');
        dbg_cnt_i    <= (others=>'0');
        fifo_din_r   <= (others=>'0');
        fifo_wr_en_r <= '0';
      else
        stb_rx       <= '0';
        fifo_wr_en_r <= '0';
 
        -- watchdog for partial pair
        if have_l='1' then
          if wd_cnt = 0 then
            have_l <= '0';
          else
            wd_cnt <= wd_cnt - 1;
          end if;
        end if;
 
        -- drop partial on stream flush/disable
        if (stream_en_s='0') or (stream_rst='1') then
          have_l <= '0';
        end if;
 
        if got_word='1' then
          if lat_a="01" then
            -- RESYNC
            have_l <= '0';
          elsif lat_a="00" then
            -- data word
            if (stream_rst='0') and (fifo_full='0') then
              if dsd_mode_s = '1' then
                -- DSD mode: store full 32 bits
                s32 := lat_d;
                if have_l='0' then
                  -- Store L as upper 32 bits
                  fifo_din_r(63 downto 32) <= s32;
                  have_l <= '1';
                  wd_cnt <= to_unsigned(PAIR_WD_CYC, wd_cnt'length);
                else
                  -- Got R, store as lower 32 bits and commit
                  fifo_din_r(31 downto 0) <= s32;
                  fifo_wr_en_r <= '1';
                  have_l       <= '0';
                  stb_rx       <= '1';
                  dbg_cnt_i    <= std_logic_vector(unsigned(dbg_cnt_i) + 1);
                end if;
              else
                -- PCM mode: extract 24 bits
                s24 := sel24(lat_d);
                if have_l='0' then
                  l_tmp  <= s24;
                  have_l <= '1';
                  wd_cnt <= to_unsigned(PAIR_WD_CYC, wd_cnt'length);
                else
                  -- got R, commit
                  l_rx         <= l_tmp;
                  r_rx         <= s24;
                  stb_rx       <= '1';
                  have_l       <= '0';
                  fifo_din_r   <= l_tmp & x"00" & s24 & x"00";
                  fifo_wr_en_r <= '1';
                  dbg_cnt_i    <= std_logic_vector(unsigned(dbg_cnt_i) + 1);
                end if;
              end if;
            end if;
          end if;
        end if;
      end if;
    end if;
  end process;
 
  fifo_din   <= fifo_din_r;
  fifo_wr_en <= fifo_wr_en_r;
 
  ---------------------------------------------------------------------------
  -- XPM FIFO async (FWFT)
  ---------------------------------------------------------------------------
  u_fifo : xpm_fifo_async
    generic map(
      FIFO_MEMORY_TYPE    => "block",
      FIFO_WRITE_DEPTH    => FIFO_DEPTH,
      WRITE_DATA_WIDTH    => W,
      READ_DATA_WIDTH     => W,
      READ_MODE           => "fwft",
      FIFO_READ_LATENCY   => 0,
      ECC_MODE            => "no_ecc",
      CDC_SYNC_STAGES     => 3,
      PROG_FULL_THRESH    => FIFO_DEPTH - FIFO_DEPTH_TRSH,
      PROG_EMPTY_THRESH   => FIFO_DEPTH_TRSH
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
      prog_empty    => fifo_prog_empty,
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
 
  ---------------------------------------------------------------------------
  -- FIFO pop pulse aligned to BCLK rising edge
  ---------------------------------------------------------------------------
  process(bclk_in)
    variable pulse : std_logic;
  begin
    if rising_edge(bclk_in) then
      if rst_n='0' then
        fifo_pop_sync <= '0';
        fifo_rd_en_r  <= '0';
      else
        pulse := fifo_pop_tgl xor fifo_pop_sync;
        fifo_pop_sync <= fifo_pop_tgl;
        fifo_rd_en_r  <= pulse;
      end if;
    end if;
  end process;
 
  -- Sync signals into BCLK domain
  process(bclk_in)
  begin
    if rising_edge(bclk_in) then
      if rst_n='0' then
        stream_rst_bff <= (others=>'0');
        stream_rst_b   <= '0';
        stream_en_bff  <= (others=>'0');
        stream_en_b    <= '0';
        dsd_mode_bff   <= (others=>'0');
        dsd_mode_b     <= '0';
      else
        stream_rst_bff(0) <= stream_rst;
        stream_rst_bff(1) <= stream_rst_bff(0);
        stream_rst_b      <= stream_rst_bff(1);
        
        stream_en_bff(0) <= stream_en_s;
        stream_en_bff(1) <= stream_en_bff(0);
        stream_en_b      <= stream_en_bff(1);
        
        dsd_mode_bff(0) <= dsd_mode_s;
        dsd_mode_bff(1) <= dsd_mode_bff(0);
        dsd_mode_b      <= dsd_mode_bff(1);
      end if;
    end if;
  end process;
 
  ---------------------------------------------------------------------------
  -- Output generator: PCM (I2S) or DSD based on mode
  ---------------------------------------------------------------------------
  process(bclk_in)
    -- PCM variables
    variable next_bit_fr : integer;
    variable next_lr     : std_logic;
    variable ch_bit      : integer;
    variable load_word   : std_logic_vector(31 downto 0);
    variable l_use, r_use: std_logic_vector(23 downto 0);
    -- DSD variables
    variable dsd_l_32, dsd_r_32 : std_logic_vector(31 downto 0);
  begin
    if falling_edge(bclk_in) then
      if rst_n='0' then
        -- PCM reset
        lr_reg       <= '0';
        bit_in_fr    <= 0;
        shreg        <= (others=>'0');
        sd_reg       <= '0';
        l_act        <= (others=>'0');
        r_act        <= (others=>'0');
        fifo_pop_tgl <= '0';
        -- DSD reset
        dsd_l_shreg  <= (others=>'0');
        dsd_r_shreg  <= (others=>'0');
        dsd_bit_cnt  <= 0;
        dsd_a_reg    <= '0';
        dsd_b_reg    <= '0';
      else
        if dsd_mode_b = '1' then
          ---------------------------------------------------------------
          -- DSD Mode: Output 1 bit per channel per clock
          --
          -- Safe/minimal fix:
          --   1) First bit is output once, then shreg is advanced immediately
          --   2) At 32-bit word boundary, if next FIFO word is not ready yet,
          --      HOLD last output level instead of forcing 0
          --   3) Clear DSD state on stream disable / stream reset
          ---------------------------------------------------------------
          if (stream_en_b = '0') or (stream_rst_b = '1') then
            dsd_l_shreg  <= (others => '0');
            dsd_r_shreg  <= (others => '0');
            dsd_bit_cnt  <= 0;
            dsd_a_reg    <= '0';
            dsd_b_reg    <= '0';

          elsif dsd_bit_cnt = 0 then
            -- Need a fresh 32-bit DSD word for each channel
            if fifo_empty = '0' then
              dsd_l_32 := fifo_dout(63 downto 32);
              dsd_r_32 := fifo_dout(31 downto 0);

              -- Output first bit now
              dsd_a_reg <= dsd_l_32(31);
              dsd_b_reg <= dsd_r_32(31);

              -- Advance immediately so next cycle outputs bit30
              dsd_l_shreg <= dsd_l_32(30 downto 0) & '0';
              dsd_r_shreg <= dsd_r_32(30 downto 0) & '0';

              fifo_pop_tgl <= not fifo_pop_tgl;

              -- 31 bits remain: bit30 .. bit0
              dsd_bit_cnt <= 31;
            else
              -- IMPORTANT: do not force zero here.
              -- Hold last output level and wait for next valid word.
              dsd_bit_cnt <= 0;
            end if;

          else
            -- Shift out remaining bits (MSB first)
            dsd_a_reg <= dsd_l_shreg(31);
            dsd_b_reg <= dsd_r_shreg(31);
            dsd_l_shreg <= dsd_l_shreg(30 downto 0) & '0';
            dsd_r_shreg <= dsd_r_shreg(30 downto 0) & '0';
            dsd_bit_cnt <= dsd_bit_cnt - 1;
          end if;

          -- Keep PCM outputs quiet
          lr_reg <= '0';
          sd_reg <= '0';
          
        else
          ---------------------------------------------------------------
          -- PCM Mode: I2S output (existing logic)
          ---------------------------------------------------------------
          if bit_in_fr = 63 then
            next_bit_fr := 0;
          else
            next_bit_fr := bit_in_fr + 1;
          end if;

          next_lr := lr_reg;
          if next_bit_fr = 0 then
            next_lr := '0';
          elsif next_bit_fr = 32 then
            next_lr := '1';
          end if;
          lr_reg <= next_lr;

          ch_bit := next_bit_fr mod 32;

          l_use := l_act;
          r_use := r_act;
          if next_bit_fr = 0 then
            if fifo_empty = '0' then
              l_use := fifo_dout(63 downto 40);
              r_use := fifo_dout(31 downto 8);
              l_act <= l_use;
              r_act <= r_use;
              fifo_pop_tgl <= not fifo_pop_tgl;
            end if;
          end if;

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
          
          -- Keep DSD outputs quiet
          dsd_a_reg <= '0';
          dsd_b_reg <= '0';
        end if;
      end if;
    end if;
  end process;
 
end architecture;
