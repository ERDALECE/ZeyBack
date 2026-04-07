library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity data_activity_blink is
  generic (
    DIV : natural := 500000  -- edge baþýna sayaç; büyüt -> daha yavaþ blink
  );
  port (
    clk_ref : in  std_logic;
    rst     : in  std_logic;     -- aktif-high reset (istersen '0' baðla)
    data_in : in  std_logic;     -- async / DATA
    led_out : out std_logic
  );
end entity;

architecture rtl of data_activity_blink is
  signal d1, d2, d3 : std_logic := '0';
  signal last       : std_logic := '0';
  signal edge_p     : std_logic := '0';

  signal acc        : unsigned(31 downto 0) := (others=>'0');
  signal led_r      : std_logic := '0';
begin

  process(clk_ref)
  begin
    if rising_edge(clk_ref) then
      if rst='1' then
        d1 <= '0'; d2 <= '0'; d3 <= '0';
        last <= '0';
        acc <= (others=>'0');
        led_r <= '0';
      else
        -- 2-3 FF senkronizasyon
        d1 <= data_in;
        d2 <= d1;
        d3 <= d2;

        -- toggle/edge algýla (DATA deðiþti mi?)
        edge_p <= '0';
        if d2 /= last then
          edge_p <= '1';
          last   <= d2;
        end if;

        -- edge biriktir, DIV'e gelince LED toggle
        if edge_p='1' then
          if acc = to_unsigned(DIV-1, acc'length) then
            acc   <= (others=>'0');
            led_r <= not led_r;
          else
            acc <= acc + 1;
          end if;
        end if;

      end if;
    end if;
  end process;

  led_out <= led_r;

end architecture;
