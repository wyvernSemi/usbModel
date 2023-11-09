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
    "hexadecimal" "",
    -default default
}
quietly virtual signal -install /test/dev_i { (concat_range (0 to 1) )( (context /test/dev_i )&{linem , linep } )} line
quietly virtual signal -install /test/host_i { (concat_range (0 to 1) )( (context /test/host_i )&{linem , linep } )} line
quietly virtual signal -install /test {/test/d  } line
quietly WaveActivateNextPane {} 0
add wave -noupdate -expand -group top /test/CLK_PERIOD_MHZ
add wave -noupdate -expand -group top /test/TIMEOUT_US
add wave -noupdate -expand -group top /test/clk
add wave -noupdate -expand -group top /test/nreset
add wave -noupdate -expand -group top /test/count
add wave -noupdate -expand -group top -radix usbline /test/line
add wave -noupdate -expand -group top -radix usbline /test/d
add wave -noupdate -expand -group host0 /test/host_i/DEVICE
add wave -noupdate -expand -group host0 /test/host_i/FULLSPEED
add wave -noupdate -expand -group host0 /test/host_i/NODENUM
add wave -noupdate -expand -group host0 /test/host_i/node
add wave -noupdate -expand -group host0 /test/host_i/clk
add wave -noupdate -expand -group host0 /test/host_i/nreset
add wave -noupdate -expand -group host0 /test/host_i/clkcount
add wave -noupdate -expand -group host0 /test/host_i/wr
add wave -noupdate -expand -group host0 -radix hexadecimal /test/host_i/wdata
add wave -noupdate -expand -group host0 /test/host_i/wack
add wave -noupdate -expand -group host0 /test/host_i/rd
add wave -noupdate -expand -group host0 -radix hexadecimal /test/host_i/rdata
add wave -noupdate -expand -group host0 /test/host_i/rack
add wave -noupdate -expand -group host0 -radix vpaddr /test/host_i/addr
add wave -noupdate -expand -group host0 /test/host_i/nopullup
add wave -noupdate -expand -group host0 /test/host_i/oen
add wave -noupdate -expand -group host0 /test/host_i/doen
add wave -noupdate -expand -group host0 /test/host_i/dp
add wave -noupdate -expand -group host0 /test/host_i/dm
add wave -noupdate -expand -group host0 -radix usbline /test/host_i/line
add wave -noupdate -expand -group host0 -radix usbline /test/host_i/linep
add wave -noupdate -expand -group host0 -radix usbline /test/host_i/linem
add wave -noupdate -group device1 /test/dev_i/DEVICE
add wave -noupdate -group device1 /test/dev_i/FULLSPEED
add wave -noupdate -group device1 /test/dev_i/GUI_RUN
add wave -noupdate -group device1 /test/dev_i/NODENUM
add wave -noupdate -group device1 /test/dev_i/node
add wave -noupdate -group device1 /test/dev_i/clk
add wave -noupdate -group device1 /test/dev_i/nreset
add wave -noupdate -group device1 /test/dev_i/clkcount
add wave -noupdate -group device1 /test/dev_i/wr
add wave -noupdate -group device1 /test/dev_i/wdata
add wave -noupdate -group device1 /test/dev_i/wack
add wave -noupdate -group device1 /test/dev_i/rd
add wave -noupdate -group device1 /test/dev_i/rack
add wave -noupdate -group device1 /test/dev_i/rdata
add wave -noupdate -group device1 -radix vpaddr /test/dev_i/addr
add wave -noupdate -group device1 /test/dev_i/nopullup
add wave -noupdate -group device1 /test/dev_i/oen
add wave -noupdate -group device1 /test/dev_i/doen
add wave -noupdate -group device1 /test/dev_i/dp
add wave -noupdate -group device1 /test/dev_i/dm
add wave -noupdate -group device1 -radix usbline /test/dev_i/line
add wave -noupdate -group device1 /test/dev_i/linep
add wave -noupdate -group device1 /test/dev_i/linem
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {18646988 ps} 0} {{Cursor 2} {7702490 ps} 0}
quietly wave cursor active 1
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
WaveRestoreZoom {0 ps} {53549353 ps}
