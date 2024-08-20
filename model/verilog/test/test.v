//=============================================================
//
// Copyright (c) 2023, 2024 Simon Southwell. All rights reserved.
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
// Top level test bench for usbModel
//-------------------------------------------------------------

module test
#(parameter CLK_PERIOD_MHZ = 12,
  parameter TIMEOUT_US     = 5000,
  parameter GUI_RUN        = 0,
  parameter VCD_DUMP       = 0,
  parameter DEBUG_STOP     = 0
);

// Derive some useful local parameters
localparam CLK_PERIOD_PS   = 1000000 / CLK_PERIOD_MHZ;
localparam TIMEOUT_COUNT   = CLK_PERIOD_MHZ * TIMEOUT_US;

reg     clk;
integer count;
wire    dp, dm;

// Generate an active low reset
wire    nreset = (count >= 10) ? 1'b1 : 1'b0;

`ifdef VERILATOR
wire    enpull;

// In verilator, pullup dp line (for FULLSPEED mode) when pullup enabled
assign (pull1, pull0) dp = enpull ? 1'b1 : 1'bZ;

`endif

// Initialise state and generate a clock
initial
begin
    clk = 1;
    
    if (VCD_DUMP)
    begin
      $dumpfile("waves.vcd");
      $dumpvars(0, test);
    end
`ifndef VERILATOR
    #0                  // Ensure first x->1 clock edge is complete before initialisation
`endif
    count = 0;
    
    // Stop the simulation when debugging to allow a debugger to connect
    if (DEBUG_STOP != 0)
    begin
      $display("\n***********************************************");
      $display("* Stopping simulation for debugger attachment *");
      $display("***********************************************\n");
      $stop;
    end

    forever # (CLK_PERIOD_PS/2) clk = ~clk;
end

// Keep a clock count and monitor for a timeout
always @(posedge clk)
begin
  count = count + 1;
  if (count >= TIMEOUT_COUNT)
  begin
    $display("***ERROR: simulation timed out");
    if (GUI_RUN==1) $stop; else $finish;
  end
end

  // ----------------------------
  // USB host
  // ----------------------------
  usbModel  #(
        .DEVICE     (0),
        .FULLSPEED  (1),
        .NODENUM    (0),
        .GUI_RUN    (GUI_RUN)
        )
  host_i
        (
        .clk         (clk),
        .nreset      (nreset),
        
`ifdef VERILATOR
        .enpull      (),
`endif

        .linep       (dp),
        .linem       (dm)
        );

  // ----------------------------
  // USB device
  // ----------------------------
  usbModel  #(
        .DEVICE     (1),
        .FULLSPEED  (1),
        .NODENUM    (1),
        .GUI_RUN    (GUI_RUN)
        )
  dev_i
        (
        .clk         (clk),
        .nreset      (nreset),
        
 `ifdef VERILATOR
        .enpull      (enpull),
`endif

        .linep       (dp),
        .linem       (dm)
        );


endmodule

