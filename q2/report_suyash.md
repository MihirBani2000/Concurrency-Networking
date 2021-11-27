# Question - 2
## How to Run
```
cd q2
make
./q2
```
## Some Points
* I have created threads for each goal that is scored in the match and for each spectator that has come to watch the match.
* I have created structs for spectators and the goals that are scored in the tournament.
* I have used `mutex locks`, `pthread_cond_wait`, `pthread_cond_broadcast` and `semaphores` alongside `pthread`s to simulate concurrency in the assignment.
* To maintain the number of remaining seats in each Zone(H/N/A), I used semaphore for each. It was initilized to the maximum capacity of each zone, and incremented/decremented as people went and came to the stadium.
## Spectator Thread
* First, I typecast the void* argument into a Person* argument.
* Now, I sleep for the time until which the person arrives to buy the tickets. After this sleep time, the person reaches at the stadium.
* Now, we do the process of **purchasing tickets**. To do this, I made use of 2 semaphores.
    * First, I initialized a clock with the absolute time of the ending of the Patience time of the person. This was done by adding the value in `tv_sec` attribute of the timespec struct.
    * Now, I use a bool `completed` and continue looping over the following until it is false.
        * At the start, I check, depending upon the kind of fan, if that spectator can get a seat in any of his eligible stadiums. Thus, for an Away spectator, I check in Away zone, for neutral fan I check in all three zones, and for home fan I check in home and neutral zones. This checking of seats in zones is done by doing **sem_trywait** on the semaphore of that zone.
        * If the person is able to lock the semaphore, then he is allotted that zone, and the `completed` bool is set `true`, and he exits the waiting loop by continuing on the loop.
        * If the person doesn't get the seat, then the person does a **sem_timedwait** on a semaphore corresponding to the kind of fan he is(H/A/N). This semaphore is initilized to 0, and acts as a signal when the seats again become available. Whenever a spectator of a corresponding zone(like H/N zone in case of H fan) gets out, then the value of that semaphore is increased. This causes it to again do the **sem_trywait** on all its available zones, and it gets the seat if the zone has a free one. This is guaranteed because after the value of semaphore would be incremented to the number of avilable seats, only that many semaphores would release their waiting lock and decrement the values. Further, the check on the individual zone's semaphores prevents accidental deadlocks from happening.
        * If the person doesn't acquire a seat until the end of his patience time, then tge `sem_timedwait` returns -1. In such a case, the person goes and waits on the gate, and is unable to get the seat since his patience time has ended.
* After this, the spectator starts watching the match.
* For neutral fans, the spectating is directly handled by making them sleep for the spectating time. After tehy wake up, they are sent to wait at the gate.
* For H/A fans, it is handled using `pthread_cond_wait`.
    * Since this waiting is dependent upon the goals scored by the opposite team, I used **pthread_cond_timedwait** on the opponent team's goals. Similarly, I used the mutex of the opponent teams goals in pthread_cond_wait.
    * Now, the code first acquires the mutex lock of the opponent team's goals.
    * Then, I loop until the spectator isn't enraged(by higher goals of other team) and the person hasn't stopped watching.
    * After this, I do a `pthread_cond_timedwait` on the `opponent_cond` variable, which is broadcasted whenever the opposite team scores a goal.
    * Now, if the opposite team scores a goal, the waiting thread is woken up, but it goes back inside the loop if the spectator isn't enraged yet. However, if the spectator is enraged, then it immediately breaks out of the loop.
    * Now, if the spectating time gets over, then the timedwait exits with a non-0 value, such that we set `person->finished_watching` to true and exit from the loop.
* Now, whenever a person who had gotten a seat goes to the gate, then:
    * we increase the semaphore value of the zone which he exited(`sem_post`).
    * Also, we `sem_post` on all the spectator semaphores(like for a N zone, we post on  H/N spectator semaphores). These are acting like a signal such that any persons who are waiting to buy tickets on those semaphores  can acquire the semaphore and get a seat in the now-emptied zone.
* We also maintain a count of the remaining members of each group, and when this value becomes 0, we make the group exit the simulation and exit the gates.

## Goal Thread
* For each goal thread, I first convert the void* pointer into a Goal* pointer.
* Then, I make the thread sleep until the time of the goal is attempted.
* When the goal is attempted, then the outcome is randomly chosen. This is done by finding a random number b/w 0 and 1, and then having the goal scored if the value is lesser than the probability.
* When a goal is scored, I increase the score of the team. Also, I broadcast a signal on the team's conditional variable, so that the spectators sleeping on the `pthread_cond_wait` wake up and check if the opposition goals have gone beyond their tolerance value.