# Create clean libraries
do cleanvlib.do

# Compile the code into the appropriate libraries
do compile_vhdl.do

# Run the tests. 
vsim -quiet -pli VProc.so test
set StdArithNoWarnings   1
set NumericStdNoWarnings 1