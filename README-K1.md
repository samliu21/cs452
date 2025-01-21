CS452 Kernel
========================================

## Make Instructions
Navigate to the root directory and call `make`. The executable is called `kernel.img` and is located in the `bin` directory.

## Available System Calls
- `create(priority, function)`: creates a new task with the specified priority that runs the given function. Returns the `tid` of the created task or `-2` if the kernel is out of task descriptors. NOTE: as of right now, there is no way to produce an invalid priority (i.e. any non-negative number is valid)
- `my_tid()`: returns the `tid` of the current task
- `my_parent_tid()`: returns the `tid` of the parent task. If the parent has exited, it returns the `tid` that the parent used to have
- `yield()`: pauses the active task and returns control to the kernel for rescheduling
- `exit()`: permanently exits a task and returns control to the kernel for rescheduling. All memory is reclaimed by the kernel 

