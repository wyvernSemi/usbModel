# Create clean libraries
do cleanvlib.do

# Compile the code into the appropriate libraries
do compile_vhdl.do

# Run the tests
vsim -gGUI_RUN=1 -gui test
set StdArithNoWarnings   1
set NumericStdNoWarnings 1
do wave.do

