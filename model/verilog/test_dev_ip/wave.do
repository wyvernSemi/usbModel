onerror {resume}
radix define vpaddr {
    "32'h00000000" "NODE#",
    "32'h00000001" "CLKCNT",
    "32'h00000002" "RSTSTATE",
    "32'h00000003" "PULLUP",
    "32'h00000004" "OEN",
    "32'h00000005" "LINE",
    "32'h000003e9" "STOP",
    "32'h000003ea" "FINISH",
    -default hexadecimal
}
radix define usbline {
    "2'b00" "SE0" -color "#ffff80",
    "2'b01" "J" -color "#00ffff",
    "2'b10" "K" -color "#80ff80",
    "2'b11" "SE1",
    "-" "color",
    "red" "2'bxx",
    " " "-color",
    "grey" "-default",
    "hexadecimal" "-default",
    "default" "-default",
    "default" "-default",
    "default" "-default",
    "default" "",
    -default default
}
quietly virtual signal -install /test/host_i { (concat_range (0 to 1) )( (context /test/host_i )&{linem , linep } )} line
quietly virtual signal -install /test {/test/d  } line
quietly WaveActivateNextPane {} 0
add wave -noupdate -expand -group top /test/CLK_PERIOD_MHZ
add wave -noupdate -expand -group top /test/TIMEOUT_US
add wave -noupdate -expand -group top /test/nreset
add wave -noupdate -expand -group top /test/count
add wave -noupdate -expand -group top /test/clk
add wave -noupdate -expand -group top /test/clk60
add wave -noupdate -expand -group top /test/usb_dp_pull
add wave -noupdate -expand -group top /test/usb_rstn
add wave -noupdate -expand -group top -radix hexadecimal /test/recv_data
add wave -noupdate -expand -group top /test/recv_valid
add wave -noupdate -expand -group top -radix hexadecimal /test/send_data
add wave -noupdate -expand -group top /test/send_data_valid
add wave -noupdate -expand -group top /test/send_ready
add wave -noupdate -expand -group top -radix usbline -childformat {{{/test/d[1]} -radix usbline} {{/test/d[0]} -radix usbline}} -subitemconfig {{/test/d[1]} {-height 15 -radix usbline} {/test/d[0]} {-height 15 -radix usbline}} /test/line
add wave -noupdate -expand -group top -radix usbline -childformat {{{/test/d[1]} -radix usbline} {{/test/d[0]} -radix usbline}} -subitemconfig {{/test/d[1]} {-height 15 -radix usbline} {/test/d[0]} {-height 15 -radix usbline}} /test/d
add wave -noupdate -group host0 /test/host_i/DEVICE
add wave -noupdate -group host0 /test/host_i/FULLSPEED
add wave -noupdate -group host0 /test/host_i/NODENUM
add wave -noupdate -group host0 /test/host_i/node
add wave -noupdate -group host0 /test/host_i/clk
add wave -noupdate -group host0 /test/host_i/nreset
add wave -noupdate -group host0 /test/host_i/clkcount
add wave -noupdate -group host0 /test/host_i/wr
add wave -noupdate -group host0 -radix hexadecimal /test/host_i/wdata
add wave -noupdate -group host0 /test/host_i/wack
add wave -noupdate -group host0 /test/host_i/rd
add wave -noupdate -group host0 -radix hexadecimal /test/host_i/rdata
add wave -noupdate -group host0 /test/host_i/rack
add wave -noupdate -group host0 -radix vpaddr /test/host_i/addr
add wave -noupdate -group host0 /test/host_i/nopullup
add wave -noupdate -group host0 /test/host_i/oen
add wave -noupdate -group host0 /test/host_i/doen
add wave -noupdate -group host0 /test/host_i/dp
add wave -noupdate -group host0 /test/host_i/dm
add wave -noupdate -group host0 -radix usbline /test/host_i/line
add wave -noupdate -group host0 -radix usbline /test/host_i/linep
add wave -noupdate -group host0 -radix usbline /test/host_i/linem
add wave -noupdate -expand -group device /test/dev_i/ASIZE
add wave -noupdate -expand -group device /test/dev_i/DEBUG
add wave -noupdate -expand -group device /test/dev_i/rstn
add wave -noupdate -expand -group device /test/dev_i/clk
add wave -noupdate -expand -group device /test/dev_i/buff
add wave -noupdate -expand -group device -radix unsigned /test/dev_i/wptr
add wave -noupdate -expand -group device -radix unsigned /test/dev_i/rptr
add wave -noupdate -expand -group device /test/dev_i/debug_data
add wave -noupdate -expand -group device /test/dev_i/debug_en
add wave -noupdate -expand -group device /test/dev_i/debug_uart_tx
add wave -noupdate -expand -group device -radix hexadecimal /test/dev_i/in_data
add wave -noupdate -expand -group device /test/dev_i/in_ready
add wave -noupdate -expand -group device /test/dev_i/in_valid
add wave -noupdate -expand -group device -radix hexadecimal /test/dev_i/recv_data
add wave -noupdate -expand -group device /test/dev_i/recv_valid
add wave -noupdate -expand -group device -radix hexadecimal /test/dev_i/send_data
add wave -noupdate -expand -group device /test/dev_i/send_ready
add wave -noupdate -expand -group device /test/dev_i/send_valid
add wave -noupdate -expand -group device /test/dev_i/usb_dn
add wave -noupdate -expand -group device /test/dev_i/usb_dp
add wave -noupdate -expand -group device /test/dev_i/usb_dp_pull
add wave -noupdate -expand -group device /test/dev_i/usb_rstn
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {919720409 ps} 0} {{Cursor 2} {174759676 ps} 0}
quietly wave cursor active 2
configure wave -namecolwidth 150
configure wave -valuecolwidth 100
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits ns
update
WaveRestoreZoom {0 ps} {1055758318 ps}
