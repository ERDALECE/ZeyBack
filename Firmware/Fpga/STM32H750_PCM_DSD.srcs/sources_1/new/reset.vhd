library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity global_reset is
    generic (
        CLK_FREQ   : integer := 50_000_000;  -- 50 MHz varsayılan clock
        RESET_TIME_MS : integer := 200       -- 200 ms reset süresi
    );
    port (
        clk      : in  std_logic;
        power_on : in  std_logic;  -- Güç açıldığında '1' olan sinyal
        reset_out: out std_logic   -- Global reset çıkışı (aktif yüksek)
    );
end global_reset;

architecture Behavioral of global_reset is
    constant COUNTER_MAX : integer := (CLK_FREQ / 1000) * RESET_TIME_MS;
    
    signal counter : integer range 0 to COUNTER_MAX := 0;
begin
    process(clk)
    begin
        if rising_edge(clk) then
            if power_on = '1' then
                if counter < COUNTER_MAX then
                    counter <= counter + 1;
                    reset_out <= '1';  -- Reset aktif
                else
                    reset_out <= '0';  -- Reset pasif
                end if;
            else
                counter <= 0;
                reset_out <= '1';  -- Güç kapalıyken reset aktif
            end if;
        end if;
    end process;
end Behavioral;