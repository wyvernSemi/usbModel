//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 29th October 2023
//
// This file is part of the usbModel package.
//
// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.
//
//=============================================================

`timescale 1ps / 1ps

//-------------------------------------------------------------
//-------------------------------------------------------------
module test
#(parameter CLK_PERIOD_MHZ = 12,
  parameter TIMEOUT_US     = 1000,
  parameter GUI_RUN        = 0
);

localparam CLK_PERIOD_PS    = 1000000 / CLK_PERIOD_MHZ;
localparam TIMEOUT_COUNT    = CLK_PERIOD_MHZ * TIMEOUT_US;

reg     clk;
integer count;
wire    d[1:0];


wire    nreset = (count >= 10) ? 1'b1 : 1'b0;

initial
begin
    clk = 1;

    #0                  // Ensure first x->1 clock edge is complete before initialisation
    count = 0;
    forever # (CLK_PERIOD_PS/2) clk = ~clk;
end

always @(posedge clk)
begin
  count = count + 1;
  if (count >= TIMEOUT_COUNT)
  begin
    $display("***ERROR: simulation timed out");
    $stop;
  end
end

  // ----------------------------
  // USB host
  // ----------------------------
  usb  #(
        .DEVICE     (0),
        .FULLSPEED  (1),
        .NODENUM    (0),
        .GUI_RUN    (GUI_RUN)
        )
  host_i
        (
        .clk         (clk),
        .nreset      (nreset),

        .linep       (d[0]),
        .linem       (d[1])
        );

  // ----------------------------
  // USB device
  // ----------------------------
  usb  #(
        .DEVICE     (1),
        .FULLSPEED  (1),
        .NODENUM    (1),
        .GUI_RUN    (GUI_RUN)
        )
  dev_i
        (
        .clk         (clk),
        .nreset      (nreset),

        .linep       (d[0]),
        .linem       (d[1])
        );


endmodule

