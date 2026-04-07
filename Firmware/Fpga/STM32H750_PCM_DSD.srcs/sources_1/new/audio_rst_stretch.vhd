library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- SR güncellemesi veya power-up sonrası audio pipeline reset'i (stretch).
-- rst_n_out async-aktif-low reset olarak kullanılabilir.
entity audio_rst_stretch is
  generic (
    HOLD_CYCLES : integer := 100000  -- 98.304MHz'de ~1.02ms
  );
  port (
    clk_ctrl  : in  std_logic;
    rst_n_in  : in  std_logic; -- global rst_n
    kick      : in  std_logic; -- 1 pulse -> reset hold başlat
    rst_n_out : out std_logic
  );
end entity;

architecture rtl of audio_rst_stretch is
  signal cnt : integer range 0 to HOLD_CYCLES := HOLD_CYCLES;
begin
  process(clk_ctrl)
  begin
    if rising_edge(clk_ctrl) then
      if rst_n_in='0' then
        cnt <= HOLD_CYCLES;
      else
        if kick='1' then
          cnt <= HOLD_CYCLES;
        elsif cnt /= 0 then
          cnt <= cnt - 1;
        end if;
      end if;
    end if;
  end process;

  rst_n_out <= '0' when (rst_n_in='0' or cnt/=0) else '1';
end architecture;
