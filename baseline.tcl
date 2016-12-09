# SDAccel command script
# Design = mul example

# Define a solution name
create_solution -name baseline_solution -dir . -force

# Define the target platform of the application
add_device -vbnv xilinx:adm-pcie-7v3:1ddr:2.1

# Host Compiler Flags
set_property -name host_cflags -value "-g -Wall -D FPGA_DEVICE"  -objects [current_solution]
# Host source files
add_files "main_tb.cpp"

# Kernel definition
create_kernel mul -type clc
add_files -kernel [get_kernels mul] "mul.cl"

# Define binary containers
create_opencl_binary mul
set_property region "OCL_REGION_0" [get_opencl_binary mul]
create_compute_unit -opencl_binary [get_opencl_binary mul] -kernel [get_kernels mul] -name K1

# Compile the design for CPU based emulation
compile_emulation -flow cpu -opencl_binary [get_opencl_binary mul]

# Run the design in CPU emulation mode
run_emulation -flow cpu -args "-a mul.xclbin -n mul"

# Generate the system estimate report
report_estimate

# Compile the application to run on the accelerator card
#build_system

# Package the application binaries
#package_system
