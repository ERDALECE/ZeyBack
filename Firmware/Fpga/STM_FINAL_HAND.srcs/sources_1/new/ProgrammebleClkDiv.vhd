library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity clkDiv is
    Port (
        clk_div: in  integer range 0 to 20000000 := 0;
		  clk_in : in  STD_LOGIC;
        reset  : in  STD_LOGIC;
        clk_out: out STD_LOGIC
    );
end clkDiv;

architecture Behavioral of clkDiv is
    signal temporal: STD_LOGIC;
    signal counter : integer range 0 to 20000000 := 0;
begin
    frequency_divider: process (reset, clk_in) begin
        if (reset = '1') then
            temporal <= '0';
            counter <= 1;
        elsif rising_edge(clk_in) then
            if (counter = clk_div) then
                temporal <= NOT(temporal);
                counter <= 1;
            else
                counter <= counter + 1;
            end if;
        end if;
    end process;
    
    clk_out <= temporal;
end Behavioral;