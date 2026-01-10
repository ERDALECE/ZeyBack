library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity i2s_tx_reclk is
  generic (
    SAMPLE_WIDTH : integer := 24  -- DAC geniþliði (ör. 24 bit)
  );
  port (
    clk        : in  std_logic;               -- bclk x 64 sistem saat
    rst        : in  std_logic;

    -- FIFO arayüzü
    fifo_empty : in  std_logic;
    fifo_data  : in  std_logic_vector(63 downto 0);
    fifo_rd    : out std_logic;
    fifo_level  : in  std_logic_vector(9 downto 0);
    -- I2S çýkýþý
    i2s_bclk   : out std_logic;
    i2s_lrclk  : out std_logic;
    i2s_data   : out std_logic
  );
end entity;

architecture rtl of i2s_tx_reclk is

  type state_type is (IDLE, READ, LOAD, SEND_L, SEND_R);
  signal state : state_type := IDLE;

  signal shreg : std_logic_vector(SAMPLE_WIDTH-1 downto 0);
  signal bit_cnt : integer range 0 to SAMPLE_WIDTH := 0;

  signal lrclk      : std_logic := '0';
  signal sdata      : std_logic := '0';
  signal fifo_rd_r  : std_logic := '0';
  signal bclk_toggle : std_logic := '0';

  signal L_word     : std_logic_vector(31 downto 0);
  signal R_word     : std_logic_vector(31 downto 0);

begin

  -- çýkýþlar
  i2s_bclk   <= clk;
  i2s_lrclk  <= lrclk;
  i2s_data   <= sdata;
  fifo_rd    <= fifo_rd_r;
  
  


  process(clk)
  begin
    if rising_edge(clk) then
      if rst = '1' then
        state      <= IDLE;
        bit_cnt    <= 0;
        fifo_rd_r  <= '0';
        lrclk      <= '0';
        sdata      <= '0';
      else
        case state is

          when IDLE =>
            fifo_rd_r <= '0';
            if to_integer(unsigned(fifo_level)) > 8 then   
              fifo_rd_r <= '1';  -- sadece 1 cycle aktif
              state <= READ;
            end if;

          when READ =>
            fifo_rd_r <= '0';
            -- Veriyi al
            L_word <= fifo_data(63 downto 32);
            R_word <= fifo_data(31 downto 0);
            state <= LOAD;

          when LOAD =>
            bit_cnt <= 0;
            shreg <= L_word(31 downto 32 - SAMPLE_WIDTH);
            lrclk <= '0';  -- sol kanal
            state <= SEND_L;

          when SEND_L =>
            sdata <= shreg(SAMPLE_WIDTH - 1 - bit_cnt);
            if bit_cnt = SAMPLE_WIDTH - 1 then
              bit_cnt <= 0;
              shreg <= R_word(31 downto 32 - SAMPLE_WIDTH);
              lrclk <= '1';  -- sað kanal
              state <= SEND_R;
            else
              bit_cnt <= bit_cnt + 1;
            end if;

          when SEND_R =>
            sdata <= shreg(SAMPLE_WIDTH - 1 - bit_cnt);
            if bit_cnt = SAMPLE_WIDTH - 1 then
              bit_cnt <= 0;
              state <= IDLE;
            else
              bit_cnt <= bit_cnt + 1;
            end if;

        end case;
      end if;
    end if;
  end process;

end architecture;

