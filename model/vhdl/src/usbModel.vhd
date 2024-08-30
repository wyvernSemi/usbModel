-- =============================================================
-- Virtual USB component
--
-- Copyright (c) 2024 Simon Southwell.
--
-- This file is part of usbModel pattern generator.
--
-- This code is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- The code is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this code. If not, see <http://www.gnu.org/licenses/>.
--
-- =============================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.env.all;

use work.usbModelPkg.all;

entity usbModel is
  generic (DEVICE         : integer := 1;  -- Select whether a device (1) or host (0)
           FULLSPEED      : integer := 1;  -- Select whether fullspeed (1) or lowspeed (0)
           NODENUM        : integer := 0;  -- Node number. Must be unique for each usbModel instantiation and any other VProc based component.
           GUI_RUN        : integer := 0   -- Flag whether running in a GUI (1) or not (0)
  );
  port    (clk            : in    std_logic;
           nreset         : in    std_logic;

           linep          : inout std_logic;
           linem          : inout std_logic
  );

end entity;


architecture behavioural of usbModel is

-- --------------------------------
-- Register definitions
-- --------------------------------

signal       oen          : std_logic := '1';
signal       nopullup     : std_logic := '1' ;
signal       dp           : std_logic := '1';
signal       dm           : std_logic := '0';

-- VP Interface signals
signal       rdata        : std_logic_vector(31 downto 0);
signal       updateresp   : std_logic := '1';

signal       clkcount     : integer := 0;

-- --------------------------------
-- Signal definitions
-- --------------------------------

signal        addr        : std_logic_vector (31 downto 0);
signal        wdata       : std_logic_vector (31 downto 0);
signal        wr          : std_logic;
signal        rd          : std_logic;
signal        wack        : std_logic := '1';
signal        rack        : std_logic := '1';
signal        node        : std_logic_vector(31 downto 0) := std_logic_vector(to_unsigned(NODENUM, 32));
signal        update      : std_logic;
signal        doen        : std_logic;

begin
-- --------------------------------
-- Combinatorial logic
-- --------------------------------

-- Speed pullup control. Device side pullup can be disconnected in highspeed mode,
-- after chirp negotiation and switch to HS, to balance line.

-- Device side pullup control (for both host and device side as VHDL does not have
-- enough levels of signal strength to have 'weak' pulldowns)

g_GEN_PULL: if DEVICE = 1 generate
linep                           <= 'H' when (FULLSPEED = 1 and nopullup = '0') else 'L';
linem                           <= 'H' when (FULLSPEED = 0 and nopullup = '0') else 'L';
end generate;

doen                            <= oen after 1 ns;

-- USB line driver logic
linep                           <= dp when (doen and oen) = '1' else 'Z';
linem                           <= dm when (doen and oen) = '1' else 'Z';

 -- --------------------------------
 -- Virtual Processor
 -- --------------------------------
  vproc_inst : entity work.VProc
  port map (Clk                 => clk,
            Addr                => addr,
            WE                  => wr,
            RD                  => rd,
            DataOut             => wdata,
            DataIn              => rdata,
            WRAck               => wack,
            RDAck               => rack,
            Interrupt           => 3x"0",
            Update              => update,
            UpdateResponse      => updateresp,
            Node                => node(3 downto 0)
           );

-- --------------------------------
-- Keep a clock tick count
-- --------------------------------
process (clk)
begin
  if clk'event and clk = '1' then
    clkcount                    <= clkcount + 1;
  end if;
end process;

-- --------------------------------
-- Process to map VProc accesses
-- to registers and simulation
-- control.
-- --------------------------------

-- Addressable read/write state from VProc
UPDATE_P : process (update)
begin

  if update'event then
  -- Default read data value
    rdata                       <= 32x"0";

    -- Process when an access is valid
    if wr = '1'or rd = '1' then

      case to_integer(unsigned(addr)) is
      when NODE_NUM    => rdata <= node;
      when CLK_COUNT   => rdata <= std_logic_vector(to_unsigned(clkcount, 32));
      when RESET_STATE => rdata <= 31x"0" & not nreset;

      when PULLUP      =>
        if wr = '1' then
          nopullup              <= not wdata(0);
        end if;
        rdata                   <= 31x"0" & nopullup;

      when OUTEN =>
        if wr = '1' then
          oen                   <= wdata(0);
        end if;
        rdata                   <= 31x"0" & oen;

      when LINE =>
        if wr = '1' then
          dp                    <= wdata(0);
          dm                    <= wdata(1);
        end if;
        rdata                   <= 30x"0" & linem & linep;

      when UVH_STOP =>
        if wr = '1' then stop ; end if;

      when UVH_FINISH =>
        -- Always stop when in GUI
        if wr = '1' then if GUI_RUN = 1 then stop; else finish; end if; end if;

      when others =>
          report UPDATE_P'path_name  & "***Error. usbModel---access to invalid address (" & to_hstring(addr) & ") from VProc" severity error;
          if GUI_RUN = 1 then stop; else finish; end if;

      end case;
    end if;

    -- Finished processing for this update, so acknowledge to VProc (by inverting updateresp)
    updateresp <= not updateresp;
  end if;
end process;

end behavioural;