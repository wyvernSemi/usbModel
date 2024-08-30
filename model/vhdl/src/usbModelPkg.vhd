-- =============================================================
-- Definitions for Virtual USB code in Modelsim
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

package usbModelPkg is

constant NODE_NUM               : integer := 0;
constant CLK_COUNT              : integer := 1;
constant RESET_STATE            : integer := 2;
constant PULLUP                 : integer := 3;
constant OUTEN                  : integer := 4;
constant LINE                   : integer := 5;

constant UVH_STOP               : integer := 1001;
constant UVH_FINISH             : integer := 1002;

end package;