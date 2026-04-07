----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 29.01.2026 18:31:48
-- Design Name: 
-- Module Name: spi_status_slave - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- ============================================================================
-- spi_status_slave (SPI slave)
--
-- Raspberry Pi SPI master -> FPGA SPI slave
--  * CDC: all status inputs are 2FF-synchronized into clk domain
--  * Snapshot: on CS assert (high->low), status is latched for that transaction
--  * Read protocol:
--      CMD byte: [7]=RW (1=READ), [6:0]=addr
--      Following bytes: if READ, data returned, addr auto-increments
--
-- Register map:
--   0x00 : ID = 0xA7
--   0x01 : status0 = [7:4]=src4, [3:0]=fs_code
--          src4 = [3]=cs_sel, [2:0]=in_sel (sel2:sel1:sel0 from STM32)
--   0x02 : status1 = [7]=is24 [6]=dsp [5]=reclk [4]=lock [3]=stream [2]=mute [1]=dsd_active [0]=cs_sel
--          dsd_active = 1 when DSD is active (your input dsd is active-low)
--   0x03 : fifo snapshot (8-bit)
--
-- SPI mode:
--   Engine is implemented as "shift in on rising edge, shift out on falling edge"
--   This matches SPI MODE 0 (CPOL=0, CPHA=0) when signals are clean.
--   If your wiring/clock domain causes phase slip, python daemon below auto-detects mode.
-- ============================================================================

entity spi_status_slave is
  port(
    clk   : in  std_logic; -- FPGA system clock (e.g. 50..200 MHz)
    rst_n : in  std_logic;

    spi_sclk : in  std_logic;
    spi_csn  : in  std_logic; -- active-low
    spi_mosi : in  std_logic;
    spi_miso : out std_logic;

    -- status inputs (may be from other clock domains)
    fs_code  : in  std_logic_vector(3 downto 0);

    -- STM32 -> FPGA:
    -- in_sel  = sel2:sel1:sel0 (3-bit mux code)
    -- cs_sel  = 1 when CS8416-family is selected (COAX/OPT1/OPT2 case)
    in_sel   : in  std_logic_vector(3 downto 0);
    dsd      : in  std_logic;  -- active-low in your system (0=DSD active)
    cs_sel   : in  std_logic;

    is24     : in  std_logic;
    dsp_on   : in  std_logic;
    reclk_on : in  std_logic;
    lock     : in  std_logic;
    stream   : in  std_logic;
    mute     : in  std_logic;

    fifo_lvl : in  std_logic_vector(7 downto 0);

    irq      : out std_logic  -- optional: toggles on status change
  );
end entity;

architecture rtl of spi_status_slave is

  -- =========================
  -- SPI pin synchronizers
  -- =========================
  signal sclk_ff : std_logic_vector(2 downto 0) := (others=>'0');
  signal cs_ff   : std_logic_vector(2 downto 0) := (others=>'1');
  signal mosi_ff : std_logic_vector(1 downto 0) := (others=>'0');

  signal sclk_s, cs_s, mosi_s : std_logic;
  signal sclk_rise, sclk_fall : std_logic;
  signal cs_assert, cs_deassert : std_logic;
  signal cs_active            : std_logic;

  -- =========================
  -- Status CDC (2FF)
  -- =========================
  signal fs_ff1, fs_ff2       : std_logic_vector(3 downto 0) := (others=>'0');
  signal in_ff1, in_ff2       : std_logic_vector(3 downto 0) := (others=>'0');
  signal fifo_ff1, fifo_ff2   : std_logic_vector(7 downto 0) := (others=>'0');

  signal dsd_ff1, dsd_ff2         : std_logic := '1';
  signal cs_sel_ff1, cs_sel_ff2   : std_logic := '0';

  signal is24_ff1, is24_ff2       : std_logic := '0';
  signal dsp_ff1, dsp_ff2         : std_logic := '0';
  signal reclk_ff1, reclk_ff2     : std_logic := '0';
  signal lock_ff1, lock_ff2       : std_logic := '0';
  signal stream_ff1, stream_ff2   : std_logic := '0';
  signal mute_ff1, mute_ff2       : std_logic := '0';

  signal dsd_active : std_logic := '0';

  signal stat0_s, stat1_s : std_logic_vector(7 downto 0);

  -- Snapshot for one SPI transaction
  signal snap_stat0 : std_logic_vector(7 downto 0) := (others=>'0');
  signal snap_stat1 : std_logic_vector(7 downto 0) := (others=>'0');
  signal snap_fifo  : std_logic_vector(7 downto 0) := (others=>'0');

  -- =========================
  -- SPI engine
  -- =========================
  signal rx_shift : std_logic_vector(7 downto 0) := (others=>'0');
  signal tx_shift : std_logic_vector(7 downto 0) := (others=>'0');
  signal miso_r   : std_logic := '0';

  signal bit_cnt  : unsigned(2 downto 0) := (others=>'0');
  signal phase    : std_logic := '0'; -- 0=command byte, 1=data bytes
  signal rw       : std_logic := '0'; -- 1=read, 0=write
  signal addr_ptr : unsigned(6 downto 0) := (others=>'0');

  -- IRQ toggle on any status change (from synced signals)
  signal prev_pack : std_logic_vector(23 downto 0) := (others=>'0');
  signal irq_r     : std_logic := '0';

    -- Pure function: reads from snapshot passed as arguments
  function rd_byte(
    a     : unsigned(6 downto 0);
    s0    : std_logic_vector(7 downto 0);
    s1    : std_logic_vector(7 downto 0);
    fifo  : std_logic_vector(7 downto 0)
  ) return std_logic_vector is
  begin
    case to_integer(a) is
      when 16#00# => return x"A7";
      when 16#01# => return s0;
      when 16#02# => return s1;
      when 16#03# => return fifo;
      when others => return (others=>'0');
    end case;
  end function;


begin
  spi_miso <= miso_r;
  irq      <= irq_r;

  -- DSD input is active-low (0=DSD active). Export dsd_active=1 when DSD.
  dsd_active <= not dsd_ff2;

  -- status0: [7:4]=src4, [3:0]=fs_code
  
  stat0_s <= in_ff2 & fs_ff2;

  -- status1: flags + dsd_active + cs_sel
  stat1_s <= is24_ff2 & dsp_ff2 & reclk_ff2 & lock_ff2 &
             stream_ff2 & mute_ff2 & dsd_active & cs_sel_ff2;

  -- =========================================================================
  -- CDC + SPI pin sync
  -- =========================================================================
  process(clk) is
    variable pack_now : std_logic_vector(23 downto 0);
  begin
    if rising_edge(clk) then
      if rst_n = '0' then
        sclk_ff <= (others=>'0');
        cs_ff   <= (others=>'1');
        mosi_ff <= (others=>'0');

        fs_ff1 <= (others=>'0'); fs_ff2 <= (others=>'0');
        in_ff1 <= (others=>'0'); in_ff2 <= (others=>'0');
        fifo_ff1 <= (others=>'0'); fifo_ff2 <= (others=>'0');

        dsd_ff1 <= '1'; dsd_ff2 <= '1';
        cs_sel_ff1 <= '0'; cs_sel_ff2 <= '0';

        is24_ff1 <= '0'; is24_ff2 <= '0';
        dsp_ff1 <= '0'; dsp_ff2 <= '0';
        reclk_ff1 <= '0'; reclk_ff2 <= '0';
        lock_ff1 <= '0'; lock_ff2 <= '0';
        stream_ff1 <= '0'; stream_ff2 <= '0';
        mute_ff1 <= '0'; mute_ff2 <= '0';

        prev_pack <= (others=>'0');
        irq_r <= '0';

      else
        -- SPI pin sync (bit0 is newest sample)
        sclk_ff <= sclk_ff(1 downto 0) & spi_sclk;
        cs_ff   <= cs_ff(1 downto 0) & spi_csn;
        mosi_ff <= mosi_ff(0) & spi_mosi;

        -- Status 2FF sync
        fs_ff1   <= fs_code;     fs_ff2   <= fs_ff1;
        in_ff1   <= in_sel;      in_ff2   <= in_ff1;
        fifo_ff1 <= fifo_lvl;    fifo_ff2 <= fifo_ff1;

        dsd_ff1 <= dsd;          dsd_ff2 <= dsd_ff1;
        cs_sel_ff1 <= cs_sel;    cs_sel_ff2 <= cs_sel_ff1;

        is24_ff1 <= is24;        is24_ff2 <= is24_ff1;
        dsp_ff1 <= dsp_on;       dsp_ff2 <= dsp_ff1;
        reclk_ff1 <= reclk_on;   reclk_ff2 <= reclk_ff1;
        lock_ff1 <= lock;        lock_ff2 <= lock_ff1;
        stream_ff1 <= stream;    stream_ff2 <= stream_ff1;
        mute_ff1 <= mute;        mute_ff2 <= mute_ff1;

        -- IRQ toggle on any change (synced)
        pack_now := stat0_s & stat1_s & fifo_ff2;
        if pack_now /= prev_pack then
          prev_pack <= pack_now;
          irq_r <= not irq_r;
        end if;
      end if;
    end if;
  end process;

  -- newest synchronized samples
  sclk_s <= sclk_ff(2);
  cs_s   <= cs_ff(2);
  mosi_s <= mosi_ff(1);

  -- edge detect using last 2 samples
  sclk_rise   <= '1' when (sclk_ff(2)='1' and sclk_ff(1)='0') else '0';
  sclk_fall   <= '1' when (sclk_ff(2)='0' and sclk_ff(1)='1') else '0';
  cs_assert   <= '1' when (cs_ff(2)='0' and cs_ff(1)='1') else '0'; -- high->low
  cs_deassert <= '1' when (cs_ff(2)='1' and cs_ff(1)='0') else '0'; -- low->high
  cs_active   <= not cs_s;

  -- =========================================================================
  -- Snapshot capture on CS assert (start of transaction)
  -- =========================================================================
  process(clk) is
  begin
    if rising_edge(clk) then
      if rst_n='0' then
        snap_stat0 <= (others=>'0');
        snap_stat1 <= (others=>'0');
        snap_fifo  <= (others=>'0');
      else
        if cs_assert='1' then
          snap_stat0 <= stat0_s;
          snap_stat1 <= stat1_s;
          snap_fifo  <= fifo_ff2;
        end if;
      end if;
    end if;
  end process;

  -- =========================================================================
  -- SPI byte engine
  --   - shift in on sclk_rise
  --   - shift out on sclk_fall
  -- =========================================================================
  process(clk) is
    variable rx_byte : std_logic_vector(7 downto 0);
  begin
    if rising_edge(clk) then
      if rst_n='0' then
        rx_shift <= (others=>'0');
        tx_shift <= (others=>'0');
        miso_r   <= '0';
        bit_cnt  <= (others=>'0');
        phase    <= '0';
        rw       <= '0';
        addr_ptr <= (others=>'0');

      elsif cs_deassert='1' then
        rx_shift <= (others=>'0');
        tx_shift <= (others=>'0');
        miso_r   <= '0';
        bit_cnt  <= (others=>'0');
        phase    <= '0';
        rw       <= '0';
        addr_ptr <= (others=>'0');

      elsif cs_active='1' then

        if sclk_fall='1' then
          miso_r   <= tx_shift(7);
          tx_shift <= tx_shift(6 downto 0) & '0';
        end if;

        if sclk_rise='1' then
          rx_shift <= rx_shift(6 downto 0) & mosi_s;

          if bit_cnt = "111" then
            rx_byte := rx_shift(6 downto 0) & mosi_s;
            bit_cnt <= (others=>'0');

            if phase='0' then
              rw       <= rx_byte(7);
              addr_ptr <= unsigned(rx_byte(6 downto 0));
              phase    <= '1';

              if rx_byte(7)='1' then
                tx_shift <= rd_byte(unsigned(rx_byte(6 downto 0)), snap_stat0, snap_stat1, snap_fifo);
              else
                tx_shift <= (others=>'0');
              end if;

            else
              if rw='1' then
                addr_ptr <= addr_ptr + 1;
                tx_shift <= rd_byte(addr_ptr + 1, snap_stat0, snap_stat1, snap_fifo);
              else
                addr_ptr <= addr_ptr + 1;
                tx_shift <= (others=>'0');
              end if;
            end if;

          else
            bit_cnt <= bit_cnt + 1;
          end if;
        end if;

      end if;
    end if;
  end process;

end architecture;