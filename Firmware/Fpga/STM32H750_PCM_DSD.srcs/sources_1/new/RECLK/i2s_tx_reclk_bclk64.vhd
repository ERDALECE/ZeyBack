library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- =============================================================================
-- i2s_tx_reclk
--   Reclocked I2S transmitter.
--   Input clock  : clk = BCLK = 64*Fs (from SI5340, single pin)
--   Output format: 32-bit slot per channel (total 64 BCLK per frame)
--
-- Notes:
-- - This module NO LONGER does any sample-rate detect / divider selection.
-- - FIFO read is prefetched near end-of-frame to tolerate 1-cycle FIFO latency.
-- - SAMPLE_WIDTH is kept for compatibility; if SAMPLE_WIDTH < 32, LSBs are zeroed.
-- =============================================================================
entity i2s_tx_reclk_raspi is
  generic (
    SAMPLE_WIDTH : integer := 24  -- meaningful bits inside 32-bit slot (MSB aligned)
  );
  port (
    clk         : in  std_logic;  -- BCLK = 64*Fs (SI5340)
    rst         : in  std_logic;

    -- FIFO interface (read clock = clk)
    fifo_empty  : in  std_logic;  -- use your prog_empty/almost_empty here
    fifo_data   : in  std_logic_vector(63 downto 0);
    fifo_rd     : out std_logic;
    fifo_level  : in  std_logic_vector(11 downto 0);

    -- I2S output
    i2s_bclk    : out std_logic;
    i2s_lrclk   : out std_logic;
    i2s_data    : out std_logic
  );
end entity;

architecture rtl of i2s_tx_reclk_raspi is
  signal bit_cnt    : unsigned(5 downto 0) := (others=>'0'); -- 0..63

  signal lrclk_r    : std_logic := '0';
  signal sdata_r    : std_logic := '0';

  signal curL       : std_logic_vector(31 downto 0) := (others=>'0');
  signal curR       : std_logic_vector(31 downto 0) := (others=>'0');

  signal nextL      : std_logic_vector(31 downto 0) := (others=>'0');
  signal nextR      : std_logic_vector(31 downto 0) := (others=>'0');
  signal have_next  : std_logic := '0';

  signal rd_pulse   : std_logic := '0';
  signal rd_pending : std_logic := '0';

  function slot32(x : std_logic_vector(31 downto 0)) return std_logic_vector is
    variable r : std_logic_vector(31 downto 0);
    variable lsb : integer;
  begin
    r := x;
    if SAMPLE_WIDTH < 32 then
      lsb := 32 - SAMPLE_WIDTH; -- number of LSBs to clear
      if lsb > 0 then
        r(lsb-1 downto 0) := (others => '0');
      end if;
    end if;
    return r;
  end function;

  function has_enough(level : std_logic_vector(11 downto 0)) return boolean is
  begin
    -- keep the old behaviour: don't start/prefetch when FIFO is near empty
    return (to_integer(unsigned(level)) > 10);
  end function;

begin
  i2s_bclk  <= clk;

  -- Main sequencer @ BCLK
  process(clk)
    variable idx : integer;
    variable Ls  : std_logic_vector(31 downto 0);
    variable Rs  : std_logic_vector(31 downto 0);
  begin
    if rising_edge(clk) then
      if rst = '1' then
        bit_cnt     <= (others=>'0');
        lrclk_r     <= '0';
        sdata_r     <= '0';

        curL        <= (others=>'0');
        curR        <= (others=>'0');
        nextL       <= (others=>'0');
        nextR       <= (others=>'0');
        have_next   <= '0';

        fifo_rd     <= '0';
        rd_pulse    <= '0';
        rd_pending  <= '0';
      else
        fifo_rd  <= '0';
        rd_pulse <= '0';

        -- ------------------------------------------------------------
        -- Prefetch next word near end of frame (tolerate 1-cycle FIFO latency)
        -- ------------------------------------------------------------
        if (bit_cnt = to_unsigned(60,6)) then
          if (fifo_empty = '0') and has_enough(fifo_level) then
            fifo_rd    <= '1';      -- pop next word
            rd_pulse   <= '1';
            rd_pending <= '1';      -- expect data valid next cycle
          end if;
        end if;

        if rd_pending = '1' then
          -- latch FIFO output as "next"
          nextL      <= slot32(fifo_data(63 downto 32));
          nextR      <= slot32(fifo_data(31 downto 0));
          have_next  <= '1';
          rd_pending <= '0';
        end if;

        -- ------------------------------------------------------------
        -- Frame boundary: swap in prefetched word
        -- ------------------------------------------------------------
        if bit_cnt = to_unsigned(63,6) then
          bit_cnt <= (others=>'0');

          if have_next = '1' then
            curL      <= nextL;
            curR      <= nextR;
            have_next <= '0';
          end if;

          lrclk_r <= '0'; -- next frame starts with Left
        else
          bit_cnt <= bit_cnt + 1;

          -- LRCLK: 0..31 left, 32..63 right
          if bit_cnt = to_unsigned(31,6) then
            lrclk_r <= '1';
          end if;
        end if;

        -- ------------------------------------------------------------
        -- SDATA (same behaviour as your old code: update on rising edge)
        -- If your DAC samples on rising edge, move this to falling edge.
        -- ------------------------------------------------------------
        idx := to_integer(bit_cnt);

        if idx < 32 then
          sdata_r <= curL(31 - idx);
        else
          sdata_r <= curR(31 - (idx - 32));
        end if;

      end if;
    end if;
  end process;

  i2s_lrclk <= lrclk_r;
  i2s_data  <= sdata_r;

end architecture;
