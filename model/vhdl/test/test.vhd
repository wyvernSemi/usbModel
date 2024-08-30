--=============================================================
--
-- Copyright (c) 2024 Simon Southwell. All rights reserved.
--
-- Date: 27th August 2024
--
-- This file is part of the usbModel package.
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
--=============================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.env.all;

-- -------------------------------------------------------------
--  Top level test bench for usbModel
-- -------------------------------------------------------------

entity test is
generic    (CLK_FREQ_MHZ   : integer   := 12;
            TIMEOUT_US     : integer   := 5000;
            GUI_RUN        : integer   := 0;
            DEBUG_STOP     : integer   := 0
);

end entity ;

architecture behavioural of test is

-- Derive some useful local constants
constant CLK_PERIOD        : time      := 1 us / CLK_FREQ_MHZ;
constant TIMEOUT_COUNT     : integer   := CLK_FREQ_MHZ * TIMEOUT_US;

signal   clk               : std_logic := '1';
signal   nreset            : std_logic := '0';
signal   count             : integer   := 0;

signal   d                 : std_logic_vector(1 downto 0);

begin
-- Generate an active low reset
nreset                     <= '1' when count >= 10 else '0';

-- -----------------------------------------------
-- Generate a clock
-- -----------------------------------------------

  P_CLKGEN : process
  begin
    -- Generate a clock cycle
    loop
      clk                              <= '1';
      wait for CLK_PERIOD/2.0;
      clk                              <= '0';
      wait for CLK_PERIOD/2.0;
    end loop;
  end process;


  -- Keep a clock count and monitor for a timeout
  process (clk)
  begin
    count <= count + 1;
    if count >= TIMEOUT_COUNT then
      report "***ERROR: simulation timed out" severity error;
      if GUI_RUN = 1 then stop; else finish; end if;
    end if;
  end process;

  -- ----------------------------
  -- USB host
  -- ----------------------------
  host_i : entity work.usbModel
    generic map (
        DEVICE      => 0,
        FULLSPEED   => 1,
        NODENUM     => 0,
        GUI_RUN     => GUI_RUN
        )
  port map (
        clk         => clk,
        nreset      => nreset,

        linep       => d(0),
        linem       => d(1)
        );

  -- ----------------------------
  -- USB device
  -- ----------------------------
  dev_i : entity work.usbModel
  generic map (
        DEVICE     => 1,
        FULLSPEED  => 1,
        NODENUM    => 1,
        GUI_RUN    => GUI_RUN
        )
  port map (
        clk         =>clk,
        nreset      =>nreset,

        linep       =>d(0),
        linem       =>d(1)
        );

end behavioural;

