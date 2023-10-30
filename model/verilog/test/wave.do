onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate -expand -group top /test/CLK_PERIOD_MHZ
add wave -noupdate -expand -group top /test/TIMEOUT_US
add wave -noupdate -expand -group top /test/clk
add wave -noupdate -expand -group top /test/nreset
add wave -noupdate -expand -group top /test/count
add wave -noupdate -expand -group top -expand /test/d
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
add wave -noupdate -group host0 -radix hexadecimal /test/host_i/addr
add wave -noupdate -group host0 /test/host_i/nopullup
add wave -noupdate -group host0 /test/host_i/oen
add wave -noupdate -group host0 /test/host_i/doen
add wave -noupdate -group host0 /test/host_i/dp
add wave -noupdate -group host0 /test/host_i/dm
add wave -noupdate -group host0 /test/host_i/linep
add wave -noupdate -group host0 /test/host_i/linem
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
add wave -noupdate -group device1 /test/dev_i/addr
add wave -noupdate -group device1 /test/dev_i/nopullup
add wave -noupdate -group device1 /test/dev_i/oen
add wave -noupdate -group device1 /test/dev_i/doen
add wave -noupdate -group device1 /test/dev_i/dp
add wave -noupdate -group device1 /test/dev_i/dm
add wave -noupdate -group device1 /test/dev_i/linep
add wave -noupdate -group device1 /test/dev_i/linem
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {25387589 ps} 0} {{Cursor 2} {166664 ps} 0}
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
WaveRestoreZoom {0 ps} {73761530 ps}
