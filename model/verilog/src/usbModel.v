// =============================================================
// Virtual USB component
//
// Copyright (c) 2023 Simon Southwell.
//
// This file is part of usbModel pattern generator.
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
// =============================================================

`timescale 1 ns / 1 ps

`include "usbModel.vh"

module usbModel
           #(parameter DEVICE    = 1,  // Select whether a device (1) or host (0)
             parameter FULLSPEED = 1,  // Select whether fullspeed (1) or lowspeed (0)
             parameter NODENUM   = 0,  // Node number. Must be unique for each usbModel instantiation and any other VProc based component.
             parameter GUI_RUN   = 0   // Flag whether running in a GUI (1) or not (0)
            )
            (
             input clk,
             input nreset,

             inout linep,
             inout linem
            );

// --------------------------------
// Register definitions
// --------------------------------
reg          oen;
reg          nopullup;
reg          dp;
reg          dm;

// VP Interface signals
reg   [31:0] rdata;
reg          updateresp;

integer      clkcount;

// --------------------------------
// Signal definitions
// --------------------------------

wire  [31:0] addr;
wire  [31:0] wdata;
wire         wr, rd;
wire         wack              = 1'b1;
wire         rack              = 1'b1;
wire  [31:0] node              = NODENUM;
wire         update;
wire         doen;

// --------------------------------
// Combinatorial logic
// --------------------------------

// Speed pullup control. Device side pullup can be disconnected in highspeed mode,
// after chirp negotiation and switch to HS, to balance line.

// Device side pullup control (explicit 1'b1/1'b0 for Icarus verilog)
assign (pull1, highz0) linep   = (DEVICE &&  FULLSPEED  && !nopullup) ? 1'b1 : 1'b0;
assign (pull1, highz0) linem   = (DEVICE && !FULLSPEED  && !nopullup) ? 1'b1 : 1'b0;

// Host side pull down resistor control
assign (highz1, weak0) linep   = (DEVICE || (nopullup)) ? 1'b1 : 1'b0;
assign (highz1, weak0) linem   = (DEVICE || (nopullup)) ? 1'b1 : 1'b0;

assign #1 doen                 = oen;

// USB line driver logic
assign linep                   = (doen & oen) ? dp : 1'bZ;
assign linem                   = (doen & oen) ? dm : 1'bZ;

// --------------------------------
// Initial process
// --------------------------------
initial
begin
  oen                          = 1'b0;
  dp                           = 1'b1;
  dm                           = 1'b0;
  nopullup                     = DEVICE ? 1'b1 : 1'b0; // Default disabled for device, enabled for host
  updateresp                   = 1'b1;
  clkcount                     = 0;
end

 // --------------------------------
 // Virtual Processor
 // --------------------------------
 VProc vp (.Clk                (clk),
           .Addr               (addr),
           .WE                 (wr),
           .RD                 (rd),
           .DataOut            (wdata),
           .DataIn             (rdata),
           .WRAck              (wack),
           .RDAck              (rack),
           .Interrupt          (3'b000),
           .Update             (update),
           .UpdateResponse     (updateresp),
           .Node               (node[3:0])
           );

// --------------------------------
// Keep a clock tick count
// --------------------------------
always @(posedge clk)
begin
  clkcount                     <= clkcount + 1;
end

// --------------------------------
// Process to map VProc accesses
// to registers and simulation
// control.
// --------------------------------

// Addressable read/write state from VProc
always @(update)
begin
  // Default read data value
  rdata                        = 32'h00000000;

  // Process when an access is valid
  if (wr === 1'b1 || rd === 1'b1)
  begin
    case(addr)
    `NODE_NUM:    rdata        = node;
    `CLKCOUNT:    rdata        = clkcount;
    `RESET_STATE: rdata        = {31'h0000, ~nreset};

    `PULLUP:
    begin
      if (wr === 1'b1)
        nopullup               = ~wdata[0];
      rdata                    = {31'h0000, nopullup};
    end

    `OUTEN:
    begin
      if (wr === 1'b1)
        oen                    = wdata[0];
      rdata                    = {31'h0000, oen};
    end

    `LINE:
    begin
      if (wr === 1'b1)
      begin
        dp                     = wdata[0];
        dm                     = wdata[1];
      end
      rdata                    = {30'h0000, linem, linep};
    end

    `UVH_STOP:
      if (wr === 1'b1) $stop;

    `UVH_FINISH:
      // Always stop when in GUI
      if (wr === 1'b1) if (GUI_RUN==1) $stop; else $finish;

    default:
    begin
        $display("%m: ***Error. usbModel---access to invalid address (%h) from VProc", addr);
        if (GUI_RUN==1) $stop; else $finish;
    end
    endcase
  end

    // Finished processing for this update, so acknowledge to VProc (by invertint updateresp)
    updateresp = ~updateresp;
end

endmodule