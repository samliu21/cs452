CS452 Kernel
========================================

## Make Instructions
Navigate to the root directory and call `make`. The executable is called `kernel.img` and is located in the `bin` directory.

### Important Make Variables
`PERFTEST`: Set this to `on` to run the performance test. Otherwise, runs the Rock-Paper-Scissors test.
`OPT`: Whether compiler optimization is on. On by default.
`MMU`: Whether the MMU is enabled. On by default.
`DCACHE`: Whether the data cache is enabled. On by default.
`ICACHE`: Whether the instruction cache is enabled. On by default.
(`DCACHE` and `ICACHE` only work if MMU is enabled)

Example:
`make clean`
`make OPT=off DCACHE=off PERFTEST=on`
Will build the image file to run the performance test with compiler optimization and the data cache disabled, and the MMU and instruction cache enabled.

Important: You must call `make clean` before re-building with different Make Variables.
