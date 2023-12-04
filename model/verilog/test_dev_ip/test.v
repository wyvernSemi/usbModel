//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 4th December 2023
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
  parameter TIMEOUT_US     = 2000,
  parameter GUI_RUN        = 0
);

// Derive some useful local parameters
localparam CLK_PERIOD_PS   = 1000000 / CLK_PERIOD_MHZ;
localparam TIMEOUT_COUNT   = CLK_PERIOD_MHZ * TIMEOUT_US;

reg          clk;
reg          clk60;
reg   [7:0]  send_data;
reg          send_data_valid;

integer      rxcount;
integer      count;

wire  [1:0]  d;
wire         send_ready;
wire  [7:0]  recv_data;
wire         recv_valid;
wire         usb_rstn;
wire         usb_dp_pull;

// --------------------------------------------------
// Generate an active low reset
// --------------------------------------------------

wire    nreset = (count >= 10) ? 1'b1 : 1'b0;

// --------------------------------------------------
// Initialise state and generate a clock for the host
// (12MHz)
// --------------------------------------------------

initial
begin
    clk = 1;

    #0                  // Ensure first x->1 clock edge is complete before initialisation
    count = 0;
    forever # (CLK_PERIOD_PS/2) clk = ~clk;
end

// --------------------------------------------------
// Generate a 60MHz clock for the device IP
// --------------------------------------------------

initial
begin
    clk60 = 1;
    rxcount = 0;

    #0                  // Ensure first x->1 clock edge is complete before initialisation
    forever # (CLK_PERIOD_PS/(2*5)) clk60 = ~clk60;
end

// --------------------------------------------------
// Keep a clock count and monitor for a timeout
// --------------------------------------------------

always @(posedge clk)
begin
  count = count + 1;
  if (count >= TIMEOUT_COUNT)
  begin
    $display("***ERROR: simulation timed out");
    if (GUI_RUN==1) $stop; else $finish;
  end
end

// --------------------------------------------------
// Emulate the pullup under control of the device
// --------------------------------------------------

assign (pull1, highz0) d[0] = usb_dp_pull;

// --------------------------------------------------
// Process for generating send data for the device
// and displaying received data.
// --------------------------------------------------

always @(posedge clk60 or negedge nreset)
begin
  if (nreset == 1'b0)
  begin
    send_data       <= 8'h00;
    send_data_valid <= 1'b0;
  end
  else
  begin
    send_data_valid <= usb_rstn;

    if (send_ready == 1'b1 && send_data_valid == 1'b1)
    begin
      send_data     <= send_data + 8'h01;
    end
    else if (usb_rstn == 1'b0)
    begin
      send_data     <= 8'h00;
    end

    // Display received data
    if (recv_valid)
    begin
      if ((rxcount % 16) == 0)
          $display("  ");

      $write("%h ", recv_data);
      rxcount      <= rxcount  + 1;
    end

  end
end

  // ----------------------------
  // USB host
  // ----------------------------
  usbModel  #(
        .DEVICE          (0),
        .FULLSPEED       (1),
        .NODENUM         (0),
        .GUI_RUN         (GUI_RUN)
        )
  host_i
        (
        .clk             (clk),
        .nreset          (nreset),

        .linep           (d[0]),
        .linem           (d[1])
        );

  // ----------------------------
  // USB device
  // ----------------------------
  usb_serial_top #(
        .DEBUG           ("FALSE")
         )
   dev_i
         (
        .rstn            (nreset),
        .clk             (clk60),

        .usb_dp_pull     (usb_dp_pull),

        .usb_dp          (d[0]),
        .usb_dn          (d[1]),

        .usb_rstn        (usb_rstn),

        .recv_data       (recv_data),
        .recv_valid      (recv_valid),

        .send_data       (send_data),
        .send_valid      (send_data_valid),
        .send_ready      (send_ready),

        .debug_en        (),
        .debug_data      (),
        .debug_uart_tx   ()
);


endmodule

