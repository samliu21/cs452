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

## Available System Calls
- `create(priority, function)`: creates a new task with the specified priority that runs the given function. Returns the `tid` of the created task or `-2` if the kernel is out of task descriptors. NOTE: as of right now, there is no way to produce an invalid priority (i.e. any non-negative number is valid)
- `my_tid()`: returns the `tid` of the current task
- `my_parent_tid()`: returns the `tid` of the parent task. If the parent has exited, it returns the `tid` that the parent used to have
- `yield()`: pauses the active task and returns control to the kernel for rescheduling
- `exit()`: permanently exits a task and returns control to the kernel for rescheduling. All memory is reclaimed by the kernel 
- `send(tid, msg, msglen, reply, rplen)`: sends a message to the task specified by `tid`. The reply from said task is placed in `reply`. Returns the size of the reply, `-1` if `tid` is invalid, and `-2` for any other error
- `receive(tid, msg, msglen)`: receives a message. The sender's ID is placed in `tid` and the sender's message in placed in `msg`. Returns the size of the sender's message
- `reply(tid, reply, rplen)`: replies to the task specified by `tid`. Returns the size of the reply message, `-1` if the task doesn't exist, and `-2` if the task exists but isn't reply-blocked
- `await_event(eventid)`: the active task blocks until the event specified by `eventid` fires. Returns event-specific data if the call succeeds and `-1` if the event is invalid

## Available Commands
- `tr <train-number> <speed>` changes the speed of the train that has number `train-number` to `speed`
- `rv <train-number>` tells the train with number `train-number` to stop, then reverse direction at its original speed
- `sw <switch-number> <direction>` tells the switch with `switch-number` to go straight with direction `S` or curved with direction `C`
- `q` restarts the program