----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 21.01.2026 02:03:17
-- Design Name: 
-- Module Name: sample_rate_dedector_lock - Behavioral
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
----------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity sample_rate_detector_lock is
  generic (
    CLK_REF_HZ    : integer := 50_000_000; -- 50/100/200 MHz
    MEAS_CYCLES   : integer := 50_000;     -- 1ms @ 50MHz
    LOCK_COUNT    : integer := 3;          -- consecutive same results to lock
    TOGGLE_FACTOR : integer := 2           -- 2 for LRCLK, 128 for BCLK (LRCLK*64)
  );
  port (
    i_clk_ref     : in  std_logic;
    i_rstb        : in  std_logic;         -- active-low
    i_sig         : in  std_logic;         -- async: LRCLK or BCLK

    SamplingFRQ   : out integer := 8;      -- 0..7 valid, 8 invalid/no_data
    SamplingFRQ_vec: out std_logic_vector (3 downto 0):="0000";
    o_lock        : out std_logic := '0';
    o_valid_p     : out std_logic := '0';  -- 1-cycle pulse when SamplingFRQ updated
    o_toggles_dbg : out unsigned(31 downto 0) := (others=>'0')
  );
end;

architecture rtl of sample_rate_detector_lock is
  -- 3FF sync
  signal s1, s2, s3 : std_logic := '0';
  signal last_s2    : std_logic := '0';

  signal tog_cnt    : unsigned(31 downto 0) := (others=>'0');
  signal win_cnt    : integer range 0 to MEAS_CYCLES := 0;

  signal last_cand  : integer range 0 to 8 := 8;
  signal same_cnt   : integer range 0 to LOCK_COUNT := 0;

  signal sr_out     : integer range 0 to 8 := 8;
  signal lock_out   : std_logic := '0';
  signal valid_p    : std_logic := '0';
  
  signal SamplingFRQ_buf   : integer range 0 to 8:=0;

  -- expected toggles = TOGGLE_FACTOR * Fs * window_time
 function exp_tog(fs_hz : integer) return integer is
  variable k    : unsigned(63 downto 0);
  variable num  : unsigned(63 downto 0);
  variable den  : unsigned(63 downto 0);
  variable q    : unsigned(63 downto 0);
begin
  -- k = TOGGLE_FACTOR * fs_hz  (128-bit çýkar, 64-bit'e resize et)
  k   := resize(to_unsigned(TOGGLE_FACTOR, 64) * to_unsigned(fs_hz, 64), 64);

  -- num = k * MEAS_CYCLES      (128-bit çýkar, 64-bit'e resize et)
  num := resize(k * to_unsigned(MEAS_CYCLES, 64), 64);

  den := to_unsigned(CLK_REF_HZ, 64);

  q := num / den;

  return to_integer(q(31 downto 0));
end function;

  constant T44  : integer := exp_tog(44_100);
  constant T48  : integer := exp_tog(48_000);
  constant T88  : integer := exp_tog(88_200);
  constant T96  : integer := exp_tog(96_000);
  constant T176 : integer := exp_tog(176_400);
  constant T192 : integer := exp_tog(192_000);
  constant T352 : integer := exp_tog(352_800);
  constant T384 : integer := exp_tog(384_000);

  constant TH0  : integer := (T44  + T48 ) / 2;
  constant TH1  : integer := (T48  + T88 ) / 2;
  constant TH2  : integer := (T88  + T96 ) / 2;
  constant TH3  : integer := (T96  + T176) / 2;
  constant TH4  : integer := (T176 + T192) / 2;
  constant TH5  : integer := (T192 + T352) / 2;
  constant TH6  : integer := (T352 + T384) / 2;

  constant MIN_TOG : integer := (T44 / 8);  -- no_data threshold (conservative)

  function classify(toggles : integer) return integer is
  begin
    if toggles < MIN_TOG then
      return 8; -- INVALID / NO_DATA
    elsif toggles < TH0 then return 0; -- 44.1
    elsif toggles < TH1 then return 1; -- 48
    elsif toggles < TH2 then return 2; -- 88.2
    elsif toggles < TH3 then return 3; -- 96
    elsif toggles < TH4 then return 4; -- 176.4
    elsif toggles < TH5 then return 5; -- 192
    elsif toggles < TH6 then return 6; -- 352.8
    else                     return 7; -- 384
    end if;
  end function;

begin
  process(i_clk_ref)
    variable toggles_i : integer;
    variable cand      : integer range 0 to 8;
  begin
    if rising_edge(i_clk_ref) then
      if i_rstb='1' then
        s1 <= '0'; s2 <= '0'; s3 <= '0';
        last_s2  <= '0';
        tog_cnt  <= (others=>'0');
        win_cnt  <= 0;

        sr_out   <= 8;
        lock_out <= '0';
        valid_p  <= '0';
        last_cand<= 8;
        same_cnt <= 0;

        o_toggles_dbg <= (others=>'0');
      else
        -- sync
        s1 <= i_sig;  s2 <= s1;  s3 <= s2;
        valid_p <= '0';

        -- toggle count
        if s2 /= last_s2 then
          tog_cnt <= tog_cnt + 1;
          last_s2 <= s2;
        end if;

        -- window end
        if win_cnt = MEAS_CYCLES-1 then
          o_toggles_dbg <= tog_cnt;
          toggles_i := to_integer(tog_cnt);
          cand := classify(toggles_i);

          if cand = 8 then
            -- fast invalid
            if sr_out /= 8 then
              sr_out  <= 8;
              valid_p <= '1';
            end if;
            lock_out <= '0';
            last_cand<= 8;
            same_cnt <= 0;
          else
            -- lock accumulation
            if cand = last_cand then
              if same_cnt < LOCK_COUNT then
                same_cnt <= same_cnt + 1;
              end if;
            else
              last_cand <= cand;
              same_cnt  <= 1;
            end if;

            if same_cnt >= LOCK_COUNT then
              if sr_out /= cand then
                sr_out  <= cand;
                valid_p <= '1';
              end if;
              lock_out <= '1';
            end if;
          end if;

          -- restart window
          tog_cnt <= (others=>'0');
          win_cnt <= 0;
        else
          win_cnt <= win_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  SamplingFRQ <= (sr_out) when (lock_out='1') else 8;  -- lock yoksa INVALID
  SamplingFRQ_buf <= (sr_out) when (lock_out='1') else 8;
  SamplingFRQ_vec <= "0000" when (SamplingFRQ_buf = 8) else
              std_logic_vector(to_unsigned(SamplingFRQ_buf + 1, 4));

o_lock      <= lock_out;
  o_valid_p   <= valid_p;

end rtl;
