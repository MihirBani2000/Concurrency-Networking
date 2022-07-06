# OSN Assignment 5 - Question 2
### ~ Mihir Bani, 2019113003

## How to Run
1. Open the `q2` directory in terminal.
2. Compile the code using ```gcc q2.c -o q2 -lpthread``` and run the code by typing ```./q2``` in terminal.

## Implementation
**Some points:**
- Separate structs are created for Persons (spectators), Goals. Global arrays are created for Groups, one containing the total number of people in it, and one containing the number of people left to leave the stadium from that group.
- **Mutex locks** are created for each group (for the number of remaining people in group), each zone (A/H/N), and each team (A/H).
- Separate threads for each Person and Goal. No thread for a Zone or Group.
- **Conditional variables** created:
  - `home_cond` and `away_cond`: for the two teams and used with `timedwait` to make the spectator wait till its time is not over or goals have not crossed threshold.
- **Semaphores** created:
  - `<zone>_sem`: for each zone A/H/N which is initiated with the max capacity of each zone. Used along with `sem_trywait` to check if seat is available or not.
  - `<zone>_cond_sem`: for each zone A/H/N which is initiated with value 0 and acts as a conditional variable. Used when spectator is waiting for a seat, and *patience* is reached. Implemented by using `timedwait`.
  
**Code flow:**
- All mutex, structs, conditional variables are initiated and input taken.
- Threads for Persons (spectators) and Goals start. In the end `pthread_join` is used only to wait for Persons threads to close. After which the program terminates.
- **Inside Persons thread:**
  - `sleep` for the required time, at time when the spectator arrives.
  - Initiate the clock by using `clock_gettime` and `timespec struct`, this will be used in `sem_timedwait`.
  - An infinite loop, to handle availing and waiting for a seat, based on the zones.
    - Check if a seat is available by using `sem_trywait`. If yes, select the seat (semaphore is decremented and locked) and exit the loop.
    - If not, wait for any available seat in feasible zones by using `sem_timedwait` till the *patience* time is not over. If time is over the the person goes and wait at the exit.
    - If a seat is freed from any required zone, a signal is recieved to the spectator and it can again `sem_trywait` for the seats in zones, which will work now, and a seat will be selected.
  - After getting a seat, the spectator watches the match.
    - *Neutral Fans:* `sleep` for *spectating time*, and then go wait at the exit gate. 
    - *Home/Away Fans:* 
      - In this case waiting depends on the goals scored by the opposite team. 
      - So locking the mutex on opponent team then starting a loop on condition what opponent goals are below threshold and time is still left. 
      - It uses `pthread_cond_timedwait` which is signaled whenever the opponent team scores a goal. If the time is over, but the goals have not cross threshold, display relevant message unlock the mutex and spectator goes to the exit gate. 
  - When the spectators reach the exit gate:
    - `sem_post` is used to signal (increment) the semaphore storing the available seats in the zone. So that the person conditionally waiting can move forward in the process. 
    - A variable stores the remaining people in the group. When all people are ready to leave, the group exits from simulation.

- **Inside Goal thread:**  
  - `sleep` for the required time, at time when the goal happens.
  - A random number is generated between 0 and 1, which is compared to the goal chance probability (lesser value of random number, means a goal happened).  
  - If goal is scored, the team goals is incremented, and signals `broadcast` to the spectators which were waiting conditionally on goals of team. This is used to check if the goal limit of spectator is reached or not, when he/she leaves the zone.
 
- If all Person threads are closed, the simulation exits.