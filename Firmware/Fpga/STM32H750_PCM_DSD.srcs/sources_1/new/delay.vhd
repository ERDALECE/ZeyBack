----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 31.03.2026 14:58:55
-- Design Name: 
-- Module Name: delay - Behavioral
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

entity relay_delay is
    generic (
        CLK_FREQ_HZ : natural := 100_000_000;
        DELAY_MS    : natural := 50              -- 40 veya 50 yapabilirsiniz
    );
    port (
        clk       : in  std_logic;
        rst       : in  std_logic;
        trig      : in  std_logic;
        relay_out : out std_logic
    );
end entity;

architecture rtl of relay_delay is
    constant DELAY_CYCLES : natural := (CLK_FREQ_HZ / 1000) * DELAY_MS;
    signal counter        : natural range 0 to DELAY_CYCLES := 0;
    signal relay_reg      : std_logic := '0';
begin

    process(clk, rst)
    begin
        if rst = '0' then
            counter   <= 0;
            relay_reg <= '1';

        elsif rising_edge(clk) then
            if trig = '1' then
                -- Anýnda kapat
                counter   <= 0;
                relay_reg <= '1';

            else
                -- trig = '1' ise gecikme say
                if counter < DELAY_CYCLES - 1 then
                    counter   <= counter + 1;
                    relay_reg <= '1';
                else
                    -- Süre doldu, röleyi çek
                    counter   <= counter;
                    relay_reg <= '0';
                end if;
            end if;
        end if;
    end process;

    relay_out <= relay_reg;

end architecture;