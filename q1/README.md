# OSN Assignment 5 - Question 1
### ~ Mihir Bani, 2019113003

## How to Run
1. Open the `q1` directory in terminal.
2. Compile the code using ```gcc q1.c -o q1 -lpthread``` and run the code by typing ```./q1``` in terminal.

## Implementation
**Some points:**
- Separate structs are created for Student, Lab and Course.
- Mutex locks are created for each student, lab and course, and also for each mentor inside a Lab.
- Separate threads for each Student and Course. No thread for a lab or TA.
- Conditional variables are created:
  - `is_ready`: for each course, if the course is allocated with TA and ready to start tut.
  - `tut_over`: for each course, whether the tut is over or not.
  - `alloted_tut`: for each student, whether he/she are alloted to a tut or not.
- Used only conditional variables. Not used Semaphores.  
  
**Code flow:**
- All mutex, structs, conditional variables are initiated and input taken.
- Threads for students and courses start. In the end `pthread_join` is used only to wait for Students threads to close. After which the program terminates.
- **Inside Student thread:**
  - A loop runs for the preferences of the student.
  - Inside this, an infinite loop runs which locks the course and has conditional wait on whether the course is ready to accept student for the tutorial or not. On recieving a signal: 
    - If the course is not removed from the simulation and is ready for tut, student is allocated in the course if the available slots are under limit. If limit is over, it starts the conditional wait again.
    - Else if the course is removed from the simulation, the preference of student is changed.
  - If the student is sitting for the tut, it has a conditional wait on whether the tut is over or not.
  - Once it recieves signal, it calculates the probability of selecting the subj permanently or changing preference.
  - If all preferences over or chosen permanently, the thread is closed by `return NULL`.
- **Inside Courses thread:**  
  - It runs an infinite loop, iterates over all the labs and their mentors sequentially, which are elligible for becoming a TA. Relevant mutex locks for labs and mentors are used.
  - If not found
    - It iterates again and again if the TA limit has not crossed, if the limit is over for the whole lab, the labs are removed. And when all labs are removed the course (with required signal broadcasted to students and variables changed) is removed and its thread closes.
  - If found  
    - it signals (`broadcast`) the students that course is ready for accepting seats and `sleeps` for some time to wait for students. It sleeps again imitating running of a tutorial.
    - Then it frees the TA, and signals the student that tut is over. It again sleeps for short duration after which it resets some variables, to be used again later.  
- If all students thread are closed, the simulation exits.
## Assumptions
- Tutorials can start with 0 students allocated. A suitable wait using `sleep` is used before starting the tutorial, while taking the tutorial, and after finishing a tutorial. [Mentioned as **Case 1** on Moodle]