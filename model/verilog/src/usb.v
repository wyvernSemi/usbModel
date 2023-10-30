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

`include "usb.vh"

module usb #(parameter DEVICE    = 1,
             parameter FULLSPEED = 1,
             parameter NODENUM   = 0,
             parameter GUI_RUN   = 0)
           (input clk,
            input nreset,
           
            inout linep,
            inout linem);


reg          oen;
reg          nopullup;
reg          dp;
reg          dm;

// VP Interface signals
reg   [31:0] rdata;
reg          updateresp;

wire  [31:0] addr;
wire  [31:0] wdata;
wire         wr, rd;
wire         wack              = 1'b1;
wire         rack              = 1'b1;
wire  [31:0] node              = NODENUM;
wire         update;

integer      clkcount;
integer      i;


initial
begin
  oen                          = 1'b0;
  dp                           = 1'b1;
  dm                           = 1'b0;
  nopullup                     = 1'b0;
  updateresp                   = 1'b1;
  clkcount                     = 0;
end

// Speed pullup control. Device side pullup can be disconnected in highspeed mode,
// after chirp negotiation and switch to HS, to balance line.

// Device side pullup control
assign (pull1, highz0) linep = DEVICE & (FULLSPEED  & !nopullup);
assign (pull1, highz0) linem = DEVICE & (!FULLSPEED & !nopullup);

// Host side pull down resistor control
assign (highz1, weak0)   linep = DEVICE | (nopullup);
assign (highz1, weak0)   linem = DEVICE | (nopullup);

wire doen;

assign #1 doen = oen;

// USB line driver logic
assign linep                   = (doen & oen) ? dp : 1'bZ;
assign linem                   = (doen & oen) ? dm : 1'bZ;

 // --------------------------------
 // Virtual Processor
 // --------------------------------
 VProc vp (.Clk            (clk), 
           .Addr           (addr), 
           .WE             (wr), 
           .RD             (rd), 
           .DataOut        (wdata), 
           .DataIn         (rdata), 
           .WRAck          (wack), 
           .RDAck          (rack), 
           .Interrupt      (3'b000), 
           .Update         (update), 
           .UpdateResponse (updateresp), 
           .Node           (node[3:0])
           );

always @(posedge clk)
begin
    clkcount     <= clkcount + 1;
end

always @(update)
begin
    rdata = 32'h00000000;
    
    if (wr === 1'b1 || rd === 1'b1)
    begin
        case(addr)
        `NODE_NUM:    rdata  = node;
        `CLKCOUNT:    rdata  = clkcount;
        `RESET_STATE: rdata  = {31'h0000, ~nreset};
        
        `PULLUP:
        begin
          if (wr === 1'b1)
            nopullup = ~wdata[0];
          rdata = {31'h0000, nopullup};
        end
        
        `OUTEN:
        begin
          if (wr === 1'b1)
            oen = wdata[0];
          rdata = {31'h0000, oen};
        end
        
        `LINE:
        begin
          if (wr === 1'b1)
          begin            
               dp = wdata[0];
               dm = wdata[1];
          end
          rdata  = {30'h0000, linem, linep};
        end

        `UVH_STOP:    if (wr === 1'b1) $stop;
        `UVH_FINISH:  if (wr === 1'b1) if (GUI_RUN) $stop; else $finish;
        
        default:
        begin
            $display("%m: ***Error. USBVhost---access to invalid address (%h) from VProc", addr);
            $stop;
        end
        endcase
    end

    // Finished processing, so flag to VProc
    updateresp = ~updateresp;
end

endmodule