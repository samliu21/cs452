# CS452

## Logistics
Sam Liu (s742liu, 20938864), Kevin Guo (k54guo, 20903138)

<a href="https://git.uwaterloo.ca/cs452-sl-kg/cs452">GitLab</a>

## K1

### Tasks
`task_t` is a task object that contains the task’s registers, ELR, SP, SPSR, and other accounting information such as ID and priority.

### Context Switching
When we enter a task from the kernel, we call the function `enter_task`, which takes as parameter pointers to the kernel and user task objects.

`enter_task` does the following operations:
- Save the kernel and user task objects onto the kernel stack.
- Save the kernel's registers in the kernel’s task object.
- Load the user task's registers, ELR, SP, and SPSR  into the registers.
- Call `eret` to jump to the address indicated by the user tasks's ELR. This should either be the entry point of the task if we’re entering it for the first time, or the task's last executed instruction otherwise.

When we switch from a task back to the kernel, we call `svc` with the appropriate syscall number. The `svc` call puts us in kernel mode. Using the previously saved pointers to the kernel and user task objects, we save the user task’s registers and restore the kernel’s registers. Finally we read ESR into `R0`, and pass this syscall number to the syscall handler.

### Syscall
When a task makes a syscall, the PC jumps back to the kernel’s main loop. The kernel grabs the syscall number from the `ESR` register, and handles the command appropriately (e.g. `create` will allocate a new task and add it to the scheduler). For syscalls like `create` and `my_tid` that have a return value, the kernel places this value in `R0`. It then schedules the next task and executes it.

### Parameters
- Max number of tasks: `64`
- Stack size: `1MB`

## K2
### Messaging
To keep track of tasks waiting to send, receive, or waiting for a reply, we use a simple queue with intrusive linking. This way, each task descriptor can store its own queue of senders waiting to message it.

For tasks waiting to receive or waiting for a reply, we just keep one queue for each state, allocated on the kernel stack.

### Name Server
The name server uses a map from strings to integers, implemented as a pair of arrays. Setting and getting an element from the map requires the entire array, but since there will not be many tasks, we believe this cost is acceptable.

## K3
### Clock Notifier
The clock notifier repeatedly calls `await_event(TICK_EVENT)`. When the syscall handler receives a `TICK_EVENT` interrupt, it notifies the clock notifier, and updates the value of `C1` to the next tick. The clock notifier then informs the clock server of the tick.

### Clock Server
The clock server maintains a `map<uint, uint>` that maps the ID of a waiting task to the time that it should be re-awakened. For example, if the `CURRENT_TICK` is `10`, and a task with tid `3` calls `delay(5)`, then the pair `<3, 15>` is placed in the map. For every tick that comes in, we iterate through the map and unblock the tasks that match the current tick. The commands `time`, `delay`, and `delay_until` are wrappers of `send`.

### Idle Measurements
The kernel keeps track of how long each task has been running for. Right before entering a task, it starts the timer, and ends the timer as soon as control is returned to the kernel. The kernel then starts its own timer, and ends the timer as soon as it enters the scheduled task.

The kernel is responsible for printing the idle measurements every 50ms. We confirmed that the time it takes to print is far less than a tick, so it’s a reasonable demand for the kernel to do occasionally. Typically, idle time stays around 99%.

## K4
### UART Notifier
There are two notifiers, one for both the terminal and the Marklin system. The UART notifiers repeatedly call `await_event` for events `EVENT_UART_TERMINAL` and `EVENT_UART_MARKLIN` respectively. These events occur when a UART interrupt comes in (e.g. write available, read available, etc.). Upon re-awakening, the notifiers call `send` to the UART server to communicate the event.

### UART Server
Similarly, there are two UART servers. Let’s first discuss the read flow. Initially, interrupts are masked. When a task calls `getc`, the UART server checks to see if a character is available to be read. If a character is available, the UART server grabs the character and returns immediately—interrupts are still disabled. If a character is not available, read interrupts are enabled, and when a UART interrupt comes in indicating that a character is available, we read the character, return it, and re-disable interrupts. Writing is quite similar to this process.

The main difference comes with the Marklin system, which also needs to track the CTS flag, which our UART server has an internal representation of. Initially, our CTS flag is set to one. Say a write request comes in and a character is put on the wire. Our CTS flag is set to zero immediately to avoid the problem caused by the CTS quirk mentioned in class. The UART server does not put any more characters on the wire until our CTS flag is set back to one. When the CTS interrupt comes in, we then set our CTS flag back to one, at which point further writes are allowed.

### Handling Commands
The `terminal_task` is responsible for repeatedly calling `getc` to take in user input. When a user types `Enter`, the terminal task grabs the command, and makes a `send` request to the `command_task`, which is responsible for actually processing the command. The `marklin_task` is responsible for actually sending commands to the Marklin system, such as setting the speed of a train.

Let’s more closely examine the `rv` command. Say `rv 55` comes in. First, we send a message to the `marklin_task` to set the speed of train 55 to zero. We then call `create` to spawn a task called `train_reverse_task`, which (1) calls `delay` to wait 3.5 seconds, (2) sends a command to reverse the train, and (3) sends a command to set the train speed back to its original speed. To get the original train speed, we make a request to `state_server`, which we discuss next. The `tr` and `sw` commands are straightforward (i.e. tell `marklin_task` to execute the command, then tell `state_server` to update the state).

### State Server
The state server maintains an internal representation of the trains, switches, and sensors. Thus, every command (e.g. `tr`, `rv`, `sw`) sends a message to the state server to inform the change in state. The `display_state_task`, which is another task that is responsible for actually printing the current state of the program in the first four lines of the terminal, makes queries to the state server to get the information it needs.

## TC

### Task Design
We have several keys tasks that communicate with each other:
- `train_task`: maintains all train state, including speed, current route, current position, intended destination, etc.
- `state_task`: maintains all track-related state, including switches, reservations, and sensors.
- `sensor_task`: repeatedly polls for sensor data and forwards it to the train task to assign to a train.
- `display_state_task`: polls all backend servers for displayable information and writes it to the console.

All tasks are set to priority one. We did not find any need to use other priorities, since we have a lot of spare idle time.

### Routing

The train is routed using Dijkstra’s algorithm. The system can forbid certain segments of the track from being used. In addition to being able to change direction at branches, we also allow the train to reverse after merges and at exits, as well as once at the beginning of its path. Once the path is calculated, the track nodes and the distances between them are stored in a `track_path` data structure, which is used by the train task.


### Stopping

To stop a train, we compute the node and offset at which the stop command should be issued, based on the stopping distance for that train. Since this stopping distance only applies for trains travelling at full speed, special handling is needed for shorter paths that aren’t long enough for the train to fully accelerate. Using our empirical values of the train’s start and stop acceleration, we can binary search the right amount of time to let the train accelerate for before issuing the stop command.

We calculate stop nodes and offsets for each reverse along the route, as well as the final stop. Furthermore, we need to consider the various offsets for each type of node we want to stop at. For example, when stopping to reverse at a merge, we need to ensure the train stops a fair distance after the node so that it can take the other branch. When reversing at an exit, we need to make sure the train stops slightly before the node.

Finally, when the model detects that the train is at the appropriate node and offset to stop at, it either creates a task to reverse the train or sends a command to stop the train, as appropriate.

### Model
Every five ticks, a notifier informs the train task to update the position model of each train. The train task uses the previous position of the train, the current speed of the train, and any acceleration periods to predict the new position of the train. We assume acceleration is constant, so the conventional kinematic equations are used.

A sensor trigger can be attributed to a train if according to the model, the train is within a reasonable distance to the sensor. Once attributed, the train’s position is updated to the position of the sensor. Effectively, sensors allow us to make real-time corrections to the internal models of the trains.

### Calibration Data

To determine train speed, we use the following loop of sensors `A3 C13 E7 D7 D9 E12 C6 B15`. We compute the distances between sensors manually and use polled sensor data to measure the time it takes for a train to move between sensors; the speed is the quotient of these two values. We allow the train to run around the loop ~10 times. We noticed that the speed of the train across different track pieces was fairly small (i.e. within ~3% of the average), so we used only the average speed in our program. 

To determine stopping distance, we issue a stop command when the train hits a specified sensor. We manually measure how far past the sensor the train reaches.

To determine starting time, we find a section of track that is overtly longer than the true starting distance. We experimentally determine the time `t` it takes for a train to traverse this section of the track from a standstill. We know there is some time `t’` at which the train stops accelerating and starts moving at a constant speed. We can use kinematic equations to determine the value of `t’`.

We put all measurements into src/train_data.c.

### Train Control

We used a reservation system to prevent collisions. The track is split up into segments, separated by branches and merges.

<img src="./docs/tc2/track_segments.png" />

Each train reserves segments as it moves, looking ahead a set distance along its path and reserving all segments in that window. After a train reaches a sensor, it releases all segments before the sensor. If a train tries to reserve a segment that is already being reserved by another train, a collision is detected.

To resolve this, both trains are stopped. Then, we calculate two possible cases: one where train one is rerouted (i.e. routed again but forbidden the conflicted segment) and train two is allowed to continue, and another where train two is rerouted and train one is allowed to continue. These two cases are compared, and we choose the one which requires the least total distance travelled by the two trains. Then the trains are restarted according to their new routes.


### Execution

During the demo, random destinations are generated for each train. Note that certain destinations are disabled (e.g. it's impossible for a train to arrive at an enter node). Moreover, we disable destinations and segments that involve dead spots in the track.
