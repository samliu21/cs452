CS452 Kernel
========================================

## Make Instructions
Navigate to the root directory and call `make`. The executable is called `kernel.img` and is located in the `bin` directory.

### Make Variables
- `MMU`: whether the MMU is enabled. On by default.
- `OPT`: whether compiler optimization is on. On by default.
- `ICACHE`: whether the instruction cache is enabled. On by default (only works if MMU is enabled).
- `DCACHE`: whether the data cache is enabled. On by default (only works if MMU is enabled).
- `PERFTEST`: Wwether the K-2 performance test should be run. Off by default.
- `MEASURE_TRAIN_SPEED`: whether the train speed should be printed. Off by default (should only be used during development for calibration).
- `TRACKA`: whether the program is run on track A. On by default.

### Example 
`make clean; make OPT=off DCACHE=off PERFTEST=on`
builds an image file to run the performance test with compiler optimization and data cache disabled, and MMU and instruction cache enabled.

Important: You must call `make clean` before re-building with different Make Variables.

## Available System Calls
- `create(priority, function)`: creates a new task with the specified priority that runs the given function. Returns the `tid` of the created task or `-2` if the kernel is out of task descriptors.
- `my_tid()`: returns the `tid` of the current task.
- `my_parent_tid()`: returns the `tid` of the parent task. If the parent has exited, it returns the `tid` that the parent used to have.
- `yield()`: pauses the active task and returns control to the kernel for rescheduling.
- `exit()`: permanently exits a task and returns control to the kernel for rescheduling. All memory is reclaimed by the kernel .
- `send(tid, msg, msglen, reply, rplen)`: sends a message to the task specified by `tid`. The reply from said task is placed in `reply`. Returns the size of the reply, `-1` if `tid` is invalid, and `-2` for any other error.
- `receive(tid, msg, msglen)`: receives a message. The sender's ID is placed in `tid` and the sender's message in placed in `msg`. Returns the size of the sender's message.
- `reply(tid, reply, rplen)`: replies to the task specified by `tid`. Returns the size of the reply message, `-1` if the task doesn't exist, and `-2` if the task exists but isn't reply-blocked.
- `await_event(eventid)`: the active task blocks until the event specified by `eventid` fires. Returns event-specific data if the call succeeds and `-1` if the event is invalid.
- `cpu_usage()`: returns the kernel and idle times as percentages.
- `terminate()`: ends execution of the kernel.

## Available Commands
- `tr <train-number> <speed>`: changes the speed of the train with number `train-number` to `speed`.
- `rv <train-number>`: tells the train with number `train-number` to stop, then reverse direction at its original speed.
- `sw <switch-number> <direction>`: tells the switch with number `switch-number` to go straight with direction `S` or curved with direction `C`.
- `go <train-number> <dest>`: changes the state of the track so that the train with number `train-number` is routed to `dest`. `dest` is specified by the `name` field in `track_data.c` (e.g. `A4`, `MR15`).
- `lp <train-number> <speed>`: redirects the train with number `train-number` onto the main loop at speed `speed`. The main loop is defined by the following sensors: `A3 C13 E7 D7 D9 E12 C6 B15`.
- `q`: restarts the program.