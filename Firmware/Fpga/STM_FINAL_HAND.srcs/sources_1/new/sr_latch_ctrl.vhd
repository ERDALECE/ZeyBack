library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- STM32'den gelen SR pinlerini (FAM, M[2:0]) SR_STB ile latch'ler.
-- Kontrol clock'u olarak sabit bir clock kullan: öneri clk_98m304.
entity sr_latch_ctrl is
  port (
    clk_ctrl   : in  std_logic;  -- sabit clock (örn 98.304 MHz)
    rst_n      : in  std_logic;

    sr_stb_in  : in  std_logic;  -- PA6 pulse
    fam_in     : in  std_logic;  -- PD0
    m_in       : in  std_logic_vector(2 downto 0); -- M2:M1:M0 = PC14:PC13:PB2

    fam_sel    : out std_logic;  -- latched family select
    rate_code  : out std_logic_vector(2 downto 0); -- latched M2:M1:M0
    upd_pulse  : out std_logic   -- 1 clk_ctrl pulse
  );
end entity;

architecture rtl of sr_latch_ctrl is
  signal stb_ff1, stb_ff2 : std_logic := '0';
  signal stb_rise         : std_logic := '0';

  signal fam_ff1, fam_ff2 : std_logic := '0';
  signal m_ff1, m_ff2     : std_logic_vector(2 downto 0) := (others => '0');

  signal fam_q            : std_logic := '0';
  signal m_q              : std_logic_vector(2 downto 0) := (others => '0');
begin
  fam_sel   <= fam_q;
  rate_code <= m_q;

  process(clk_ctrl)
  begin
    if rising_edge(clk_ctrl) then
      if rst_n='0' then
        stb_ff1 <= '0'; stb_ff2 <= '0';
        fam_ff1 <= '0'; fam_ff2 <= '0';
        m_ff1   <= (others => '0'); m_ff2 <= (others => '0');
        fam_q   <= '0';
        m_q     <= (others => '0');
        upd_pulse <= '0';
      else
        -- sync inputs
        stb_ff1 <= sr_stb_in;  stb_ff2 <= stb_ff1;
        fam_ff1 <= fam_in;     fam_ff2 <= fam_ff1;
        m_ff1   <= m_in;       m_ff2   <= m_ff1;

        stb_rise  <= (stb_ff1 and (not stb_ff2));
        upd_pulse <= '0';

        if stb_rise='1' then
          fam_q    <= fam_ff2;
          m_q      <= m_ff2;
          upd_pulse <= '1';
        end if;
      end if;
    end if;
  end process;
end architecture;
