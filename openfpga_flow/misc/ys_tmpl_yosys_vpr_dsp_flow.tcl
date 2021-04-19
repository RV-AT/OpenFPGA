# Yosys synthesis script for ${TOP_MODULE}
yosys -import

#########################
# Parse input files
#########################
# Read verilog files
${READ_VERILOG_FILE}
# Read technology library
read_verilog -lib -specify ${YOSYS_CELL_SIM_VERILOG}

#########################
# Prepare for synthesis
#########################
# Identify top module from hierarchy
hierarchy -check -top ${TOP_MODULE}
# - Convert process blocks to AST
procs
# Flatten all the gates/primitives
flatten
# Identify tri-state buffers from 'z' signal in AST
# with follow-up optimizations to clean up AST
tribuf -logic
opt_expr
opt_clean
# demote inout ports to input or output port
# with follow-up optimizations to clean up AST
deminout
opt

opt_expr
opt_clean
check
opt
wreduce -keepdc
peepopt
pmuxtree
opt_clean

########################
# Map multipliers
# Inspired from synth_xilinx.cc
#########################
# Avoid merging any registers into DSP, reserve memory port registers first
memory_dff
wreduce t:\$mul
techmap -map +/mul2dsp.v -map ${YOSYS_DSP_MAP_VERILOG} ${YOSYS_DSP_MAP_PARAMETERS}
select a:mul2dsp
setattr -unset mul2dsp
opt_expr -fine
wreduce
select -clear
chtype -set \$mul t:\$__soft_mul # Extract arithmetic functions

#########################
# Run coarse synthesis
#########################
# Run a tech map with default library
techmap
alumacc
share
opt
fsm
# Run a quick follow-up optimization to sweep out unused nets/signals
opt -fast
# Optimize any memory cells by merging share-able ports and collecting all the ports belonging to memorcy cells  
memory -nomap
opt_clean

#########################
# Map flip-flops
#########################
techmap -map +/adff2dff.v
opt_expr -mux_undef
simplemap
opt_expr
opt_merge
opt_rmdff
opt_clean
opt

#########################
# Map LUTs
#########################
abc -lut ${LUT_SIZE}

#########################
# Check and show statisitics
#########################
hierarchy -check
stat

#########################
# Output netlists
#########################
opt_clean -purge
write_blif ${OUTPUT_BLIF}