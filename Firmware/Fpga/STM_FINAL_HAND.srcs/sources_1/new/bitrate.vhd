library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity bitrate is
    Port ( D   : in  STD_LOGIC;
           CLK : in  STD_LOGIC;
           Q   : out STD_LOGIC_VECTOR (31 downto 0));
end bitrate;

architecture Behavioral of bitrate is
     signal shift_reg : STD_LOGIC_VECTOR(31 downto 0) := "00000000000000000000000000000000";
begin

  
    process (CLK)
    begin
        if rising_edge(CLK) then
            shift_reg <= shift_reg(30 downto 0) & D;
        end if;
    end process;
    
    -- hook up the shift register bits to the LEDs
    Q <= shift_reg;

end Behavioral;