library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- ============================================================================
-- stm8_parallel_rx_toggle_sync_req (enhanced)
--   - en_stream input: gates streaming; forces idle when 0
--   - SYNC pulse per sample (sync_in): starts a sample, resets byte counter
--   - Toggle strobe per byte (stm_tgl_in)
--   - REQ output to STM32 for sample-level flow control
--   - Byte validate: 3-point majority sampling around the strobe event
--
-- Notes:
--   * sys_clk = 100 MHz recommended.
--   * Choose SYNC_GUARD small (0..4). This is NOT a watchdog.
--   * Byte validate adds ~tens of ns latency but completes well within a byte period.
-- ============================================================================

entity stm8_parallel_rx_toggle_sync_req is
  generic (
    CAP_DLY        : integer := 0;  -- 0..3 data pipeline tap (sys_clk cycles)
    SYNC_GUARD     : integer := 2;  -- ignore toggle events for N sys_clk after SYNC (data settle)
    VAL_WAIT1_CYC  : integer := 1;  -- cycles after strobe-event before sample #1
    VAL_WAIT2_CYC  : integer := 1;  -- cycles between sample #1 and #2
    VAL_WAIT3_CYC  : integer := 1   -- cycles between sample #2 and #3
  );
  port (
    sys_clk        : in  std_logic;
    rst_n          : in  std_logic;

    en_stream      : in  std_logic;  -- 1=enable streaming (STM32 PD4 from g_audio_run)

    -- STM -> FPGA
    sync_in        : in  std_logic;  -- SYNC pulse per sample (can stay on PC7 like your working setup)
    stm_tgl_in     : in  std_logic;  -- toggle per byte (strobe)
    data_in        : in  std_logic_vector(7 downto 0);

    -- FPGA -> STM
    fifo_prog_full : in  std_logic;  -- throttle input (near full)
    fpga_req_out   : out std_logic;  -- REQ (ready-for-new-sample)

    -- output sample
    sample_l       : out std_logic_vector(23 downto 0);
    sample_r       : out std_logic_vector(23 downto 0);
    sample_stb     : out std_logic
  );
end entity;

architecture rtl of stm8_parallel_rx_toggle_sync_req is

  -- data pipeline
  type t_pipe is array (0 to 3) of std_logic_vector(7 downto 0);
  signal dpipe : t_pipe := (others => (others=>'0'));

  function pick_data(pipe : t_pipe; dly : integer) return std_logic_vector is
  begin
    if dly <= 0 then return pipe(0);
    elsif dly = 1 then return pipe(1);
    elsif dly = 2 then return pipe(2);
    else return pipe(3);
    end if;
  end function;

  -- sync + edge detect for SYNC pulse
  signal sync_ff  : std_logic_vector(2 downto 0) := (others=>'0');
  signal sync_evt : std_logic := '0';

  -- sync + toggle event for byte strobe
  signal tgl_ff  : std_logic_vector(2 downto 0) := (others=>'0');
  signal tgl_evt : std_logic := '0';

  -- receiver state
  signal busy      : std_logic := '0';
  signal cnt       : unsigned(2 downto 0) := (others=>'0'); -- 0..5
  signal guard_cnt : unsigned(7 downto 0) := (others=>'0');

  signal lreg, rreg : std_logic_vector(23 downto 0) := (others=>'0');

  -- byte validate mini-FSM
  type vstate_t is (V_IDLE, V_W1, V_W2, V_W3);
  signal vstate   : vstate_t := V_IDLE;
  signal vcnt     : unsigned(7 downto 0) := (others=>'0');
  signal d1, d2, d3 : std_logic_vector(7 downto 0) := (others=>'0');

  -- optional debug counter (not exported)
  signal val_mismatch_cnt : unsigned(15 downto 0) := (others=>'0');

  function majority3(a,b,c : std_logic_vector(7 downto 0)) return std_logic_vector is
  begin
    if (a = b) or (a = c) then
      return a;
    elsif (b = c) then
      return b;
    else
      return c;
    end if;
  end function;

begin
  sample_l <= lreg;
  sample_r <= rreg;

  -- REQ is "ready for a NEW sample"
  fpga_req_out <= '1' when (en_stream='1' and rst_n='1' and busy='0' and fifo_prog_full='0') else '0';

  process(sys_clk)
    variable db : std_logic_vector(7 downto 0);
  begin
    if rising_edge(sys_clk) then
      if rst_n='0' then
        dpipe      <= (others => (others=>'0'));
        sync_ff    <= (others=>'0');
        sync_evt   <= '0';
        tgl_ff     <= (others=>'0');
        tgl_evt    <= '0';

        busy       <= '0';
        cnt        <= (others=>'0');
        guard_cnt  <= (others=>'0');

        lreg       <= (others=>'0');
        rreg       <= (others=>'0');
        sample_stb <= '0';

        vstate     <= V_IDLE;
        vcnt       <= (others=>'0');
        d1         <= (others=>'0');
        d2         <= (others=>'0');
        d3         <= (others=>'0');
        val_mismatch_cnt <= (others=>'0');

      else
        sample_stb <= '0';

        -- pipeline data
        dpipe(0) <= data_in;
        dpipe(1) <= dpipe(0);
        dpipe(2) <= dpipe(1);
        dpipe(3) <= dpipe(2);

        -- sync inputs
        sync_ff(0) <= sync_in;     sync_ff(1) <= sync_ff(0);     sync_ff(2) <= sync_ff(1);
        tgl_ff(0)  <= stm_tgl_in;  tgl_ff(1)  <= tgl_ff(0);      tgl_ff(2)  <= tgl_ff(1);

        sync_evt <= sync_ff(2) and not sync_ff(1);  -- rising edge
        tgl_evt  <= tgl_ff(2) xor tgl_ff(1);        -- any toggle

        -- hard gate
        if en_stream='0' then
          busy      <= '0';
          cnt       <= (others=>'0');
          guard_cnt <= (others=>'0');
          vstate    <= V_IDLE;
          vcnt      <= (others=>'0');

        else
          -- Start of a new sample on SYNC pulse (only if not busy and not throttled)
          if (sync_evt='1') then
            if (busy='0' and fifo_prog_full='0') then
              busy <= '1';
              cnt  <= (others=>'0');
              vstate <= V_IDLE;  -- reset validator for clean start

              if SYNC_GUARD <= 0 then
                guard_cnt <= (others=>'0');
              else
                guard_cnt <= to_unsigned(SYNC_GUARD, guard_cnt'length);
              end if;
            end if;
          end if;

          -- guard countdown
          if busy='1' then
            if guard_cnt /= 0 then
              guard_cnt <= guard_cnt - 1;
            end if;
          end if;

          -- Byte validate FSM runs only while busy and after guard
          if (busy='1' and guard_cnt=0) then

            case vstate is
              when V_IDLE =>
                if tgl_evt='1' then
                  vstate <= V_W1;
                  if VAL_WAIT1_CYC <= 0 then
                    vcnt <= (others=>'0');
                  else
                    vcnt <= to_unsigned(VAL_WAIT1_CYC, vcnt'length);
                  end if;
                end if;

              when V_W1 =>
                if vcnt = 0 then
                  d1 <= pick_data(dpipe, CAP_DLY);
                  vstate <= V_W2;
                  if VAL_WAIT2_CYC <= 0 then
                    vcnt <= (others=>'0');
                  else
                    vcnt <= to_unsigned(VAL_WAIT2_CYC, vcnt'length);
                  end if;
                else
                  vcnt <= vcnt - 1;
                end if;

              when V_W2 =>
                if vcnt = 0 then
                  d2 <= pick_data(dpipe, CAP_DLY);
                  vstate <= V_W3;
                  if VAL_WAIT3_CYC <= 0 then
                    vcnt <= (others=>'0');
                  else
                    vcnt <= to_unsigned(VAL_WAIT3_CYC, vcnt'length);
                  end if;
                else
                  vcnt <= vcnt - 1;
                end if;

              when V_W3 =>
                if vcnt = 0 then
                  d3 <= pick_data(dpipe, CAP_DLY);

                  db := majority3(d1, d2, d3);
                  if ( (d1 /= d2) and (d1 /= d3) and (d2 /= d3) ) then
                    val_mismatch_cnt <= val_mismatch_cnt + 1;
                  end if;

                  -- commit byte
                  case cnt is
                    when "000" => lreg(23 downto 16) <= db; cnt <= cnt + 1;
                    when "001" => lreg(15 downto 8)  <= db; cnt <= cnt + 1;
                    when "010" => lreg(7 downto 0)   <= db; cnt <= cnt + 1;
                    when "011" => rreg(23 downto 16) <= db; cnt <= cnt + 1;
                    when "100" => rreg(15 downto 8)  <= db; cnt <= cnt + 1;
                    when "101" =>
                      rreg(7 downto 0) <= db;
                      cnt <= (others=>'0');
                      busy <= '0';
                      sample_stb <= '1';
                    when others =>
                      cnt <= (others=>'0');
                      busy <= '0';
                  end case;

                  vstate <= V_IDLE;
                  vcnt   <= (others=>'0');
                else
                  vcnt <= vcnt - 1;
                end if;

            end case;

          else
            vstate <= V_IDLE;
            vcnt   <= (others=>'0');
          end if;

        end if;
      end if;
    end if;
  end process;

end architecture;




