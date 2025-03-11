## Trac-man

### Group

Sam Liu, Kevin Guo

### Overview

There is initially one Trac-man and at least one Trac-ghost, each represented by trains. At the start of execution, the trains are navigating to their appropriate starting positions. On the UI, the user can see the position of the trains and the Trac-dots. The objective is to eat all the Trac-dots while avoiding the Trac-ghosts.

Once the game begins, the user uses arrow keys to control the Trac-man. The forward key makes the train move forward, the back key makes the train reverse, and the left and right arrow keys are used to traverse branches. The Trac-ghosts follow an algorithm in an attempt to catch the Trac-man. 

### Challenges

- Allowing a user-controlled Trac-man. For example, the player should be able to user arrow keys to move the train forward, reverse the train, and choose a branch to take.
- Develop a real-time routing algorithm for the Trac-ghosts.
- Create a system to maintain game state. This includes creating a UI for the user, starting and ending the game, auto-generating the Trac-dots, etc.

### Solutions

- To capture forward movement, we continue to allow the train to move at its current speed while forward keys are being pressed at a sufficient frequency. Otherwise, we send a stop command so the train begins deccelerating. Assuming a constant acceleration, it should not be a challenge to maintain our internal model of the train's position. When a train is approaching a branch, we indicate to the user a branch choice and set the switch accordingly.
- A basic algorithm is to route the Trac-ghosts to the current position of the Trac-man. Upon reaching this destination, the Trac-ghost is re-routed to the new position of the Trac-man, without stopping. Different Trac-ghosts should have different behaviours to avoid interfering with each other. A more advanced algorithm might take into account the expected position of the Trac-man.
- Develop a real-time UI for the game using ANSI-codes. We should avoid the overly conservative reservation process from train control since upon collision, we can end the game. 
