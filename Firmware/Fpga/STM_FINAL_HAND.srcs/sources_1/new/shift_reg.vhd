library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity shift_reg is
    Port ( D   : in  STD_LOGIC;
           CLK : in  STD_LOGIC;
           Q   : out STD_LOGIC_VECTOR (8 downto 0));
end shift_reg;

architecture Behavioral of shift_reg is
     signal shift_reg : STD_LOGIC_VECTOR(8 downto 0) := "000000000";
begin

  
    process (CLK)
    begin
        if (CLK'event and CLK = '1') then
            shift_reg(7 downto 0) <= shift_reg(8 downto 1);
            shift_reg(8) <= D;
        end if;
    end process;
    
    -- hook up the shift register bits to the LEDs
    Q <= shift_reg;

end Behavioral;
   