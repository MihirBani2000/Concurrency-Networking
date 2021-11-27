# OSN Assignment 5 - Question 2
### ~ Mihir Bani, 2019113003

## How to Run
1. Open the `q2` directory in terminal.
2. Compile the code using ```gcc q2.c -o q2 -lpthread``` and run the code by typing ```./q2``` in terminal.

## Implementation
**Some points:**
- Separate structs are created for Persons (spectators), Goals. Global arrays are created for Groups, one containing the total number of people in it, and one containing the number of people left to leave the stadium from that group.
- Mutex locks are created for each group (for the number of remaining people in group), each zone (A/H/N), and each team (A/H).
- Separate threads for each Person and Goal. No thread for a Zone or Group.
- Conditional variables created:
  - `home_cond` and `away_cond`: for the two teams and used with `timedwait` to make the spectator wait till its time is not over or goals have not crossed threshold.
- Semaphores created:
  - hi
  
**Code flow:**
- All mutex, structs, conditional variables are initiated and input taken.
- Threads for Persons (spectators) and Goals start. In the end `pthread_join` is used only to wait for Persons threads to close. After which the program terminates.
- **Inside Persons thread:**

- **Inside Goal thread:**  
 
- If all Person threads are closed, the simulation exits.
## Assumptions
- 