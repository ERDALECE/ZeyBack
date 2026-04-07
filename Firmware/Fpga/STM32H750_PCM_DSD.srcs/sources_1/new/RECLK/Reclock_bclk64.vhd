------ Spdif Re_clock Module -----
------ i2s in i2s out ------------
------ ERDAL TÜRKEKUL ------------
---------- 2025 ------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library xpm;
use xpm.vcomponents.all;

entity i2s_reclock_raspi is
  port (
    -- SI5340 / reference clock for reclocked output domain
    si_clk        : in  std_logic; -- e.g. 22.5792MHz
    rst           : in  std_logic;
    reclk_enb     : in  std_logic;

    -- Input I2S from RasPi (master)
    i2s_in_bclk   : in  std_logic;
    i2s_in_lrclk  : in  std_logic;
    i2s_in_sdata  : in  std_logic;

    -- Output I2S (reclocked with si_clk when reclk_enb=1)
    i2s_out_bclk  : out std_logic;
    i2s_out_lrclk : out std_logic;
    i2s_out_sdata : out std_logic
  );
end entity;

architecture Structural of i2s_reclock_raspi is

  -- =============================
  -- FIFO config
  -- =============================
  constant FIFO_DEPTH   : integer := 2048;
  constant W            : integer := 64;
  constant CNT_W        : integer := 12; -- log2(2048)+1
  constant SAMPLE_WIDTH : integer := 32; -- 32-bit slot

  -- I2S RX parallel outputs
  signal L_data_in      : std_logic_vector(31 downto 0);
  signal R_data_in      : std_logic_vector(31 downto 0);
  signal L_data_ready   : std_logic;
  signal R_data_ready   : std_logic;

  -- buffers (keep your original names)
  signal L_data_in_buf  : std_logic_vector(31 downto 0) := (others=>'0');
  signal R_data_in_buf  : std_logic_vector(31 downto 0) := (others=>'0');

  -- FIFO signals (keep names)
  signal fifo_wr_en : std_logic := '0';
  signal fifo_rd_en : std_logic := '0';
  signal fifo_din   : std_logic_vector(63 downto 0) := (others=>'0');
  signal fifo_dout  : std_logic_vector(63 downto 0);

  signal fifo_level : std_logic_vector(11 downto 0); -- CNT_W
  signal almost_full_custom  : std_logic;
  signal almost_empty_custom : std_logic;

  -- IMPORTANT: real empty flag for TX
  signal fifo_empty_sig : std_logic;

  -- output buffers
  signal i2s_out_sdata_buf : std_logic := '0';
  signal i2s_out_lrclk_buf : std_logic := '0';
  signal i2s_out_bclk_buf  : std_logic := '0';

  -- (sen eski kodda lockedpll <= not rst yapýyordun)
  signal lockedpll : std_logic;

begin

  lockedpll <= not rst;

  -- ============================================================
  -- I2S Receiver (RasPi clock domain: i2s_in_bclk)
  -- ============================================================
  i2s_rx_rclk: entity work.i2s_in
    generic map (
      width => SAMPLE_WIDTH
    )
    port map(
      -- I2S ports
      LR_CLK      => i2s_in_lrclk,
      BIT_CLK     => i2s_in_bclk,
      DIN         => i2s_in_sdata,

      -- Control
      RESET_I     => lockedpll,

      -- Parallel
      DATA_L      => L_data_in,
      DATA_R      => R_data_in,

      -- Status
      DATA_RDY_L  => L_data_ready,
      DATA_RDY_R  => R_data_ready
    );

  -- ============================================================
  -- Write FIFO in i2s_in_bclk domain (same as your original style)
  -- Strategy: store R on R_ready, write L+Rbuf when L_ready
  -- ============================================================
  i2s_write_fifo: process (i2s_in_bclk)
  begin
    if rising_edge(i2s_in_bclk) then
      fifo_wr_en <= '0';

      if R_data_ready = '1' then
        R_data_in_buf <= R_data_in;
      end if;

      -- when Left arrives, write stereo (L current + R previous)
      if (L_data_ready = '1') and (almost_full_custom = '0') then
        fifo_din   <= L_data_in & R_data_in_buf;
        fifo_wr_en <= '1';
      end if;
    end if;
  end process;

  -- ============================================================
  -- XPM FIFO ASYNC (2048 x 64)
  -- prog_empty -> almost_empty_custom (<=64 words)
  -- prog_full  -> almost_full_custom  (>=1984 words)
  -- rd_data_count -> fifo_level (12-bit)
  -- ============================================================
  fifo_rclk : xpm_fifo_async
    generic map(
      FIFO_MEMORY_TYPE      => "block",
      FIFO_WRITE_DEPTH      => FIFO_DEPTH,

      WRITE_DATA_WIDTH      => W,
      READ_DATA_WIDTH       => W,

      READ_MODE             => "fwft",
      FIFO_READ_LATENCY     => 0,

      CDC_SYNC_STAGES       => 2,
      RELATED_CLOCKS        => 0,
      ECC_MODE              => "no_ecc",

      PROG_EMPTY_THRESH     => 64,
      PROG_FULL_THRESH      => 1984,

      WR_DATA_COUNT_WIDTH   => CNT_W,
      RD_DATA_COUNT_WIDTH   => CNT_W,

      USE_ADV_FEATURES      => "0707"
    )
    port map(
      rst           => rst,

      -- WRITE side (RasPi BCLK domain)
      wr_clk        => i2s_in_bclk,
      wr_en         => fifo_wr_en,
      din           => fifo_din,
      full          => open,
      prog_full     => almost_full_custom,
      wr_data_count => open,
      overflow      => open,
      wr_rst_busy   => open,

      -- READ side (SI clock domain)
      rd_clk        => si_clk,
      rd_en         => fifo_rd_en,
      dout          => fifo_dout,
      empty         => fifo_empty_sig,       -- REAL empty
      prog_empty    => almost_empty_custom,  -- near-empty threshold
      rd_data_count => fifo_level,
      underflow     => open,
      rd_rst_busy   => open,

      -- unused outputs
      data_valid    => open,
      almost_empty  => open,
      almost_full   => open,

      -- ECC / sleep disabled
      sleep         => '0',
      injectsbiterr => '0',
      injectdbiterr => '0',
      sbiterr       => open,
      dbiterr       => open
    );

  -- ============================================================
  -- I2S Transmitter (reclocked output, si_clk domain)
  -- IMPORTANT: fifo_empty gets REAL empty, not prog_empty
  -- ============================================================
  i2s_tx_reclk_inst : entity work.i2s_tx_reclk_raspi
    generic map (
      SAMPLE_WIDTH => SAMPLE_WIDTH
    )
    port map (
      clk        => si_clk,
      rst        => rst,
      fifo_empty => fifo_empty_sig,
      fifo_data  => fifo_dout,
      fifo_rd    => fifo_rd_en,
      fifo_level => fifo_level,
      i2s_bclk   => i2s_out_bclk_buf,
      i2s_lrclk  => i2s_out_lrclk_buf,
      i2s_data   => i2s_out_sdata_buf
    );

  -- ============================================================
  -- Output mux (unchanged)
  -- ============================================================
  i2s_out_bclk  <= i2s_out_bclk_buf  when reclk_enb = '1' else i2s_in_bclk;
  i2s_out_lrclk <= i2s_out_lrclk_buf when reclk_enb = '1' else i2s_in_lrclk;
  i2s_out_sdata <= i2s_out_sdata_buf when reclk_enb = '1' else i2s_in_sdata;

end Structural;
