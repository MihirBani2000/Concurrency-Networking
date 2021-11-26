#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

//Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

// some constants
#define MAX_MENTORS 100
#define MAX_LABS 100
#define MAX_STUDENTS 100
#define MAX_COURSES 100
#define MAX_STRING 100
#define NUM_PREFS 3
#define PROB 0.5

// struct for student
struct Student
{
    int id;                     // -1 if not valid
    int arrival_time;           // time after it comes
    int curr_pref;              // stores the current preference, 0/1/2
    int preferences[NUM_PREFS]; // list storing the 1st, 2nd, 3rd subj pref
    float calibre;
    pthread_cond_t alloted_tut;
};

// struct for lab
struct Lab
{
    char name[MAX_STRING];
    int id;                                // -1 if not valid
    int num_students;                      // students associated with it, to be TA mentors
    int ta_limit;                          // limit for each ta
    int ta_limit_array[MAX_MENTORS];       // stores the number of times left a mentor can become ta
    int ta_occupied_array[MAX_MENTORS];    // stores if mentor is busy
    pthread_mutex_t ta_mutex[MAX_MENTORS]; // locks for each TA
};

// struct for course
struct Course
{
    char name[MAX_STRING];
    int id;                  // -1 if not valid
    float interest;          // the interest value of the course
    int max_slots;           // the max limit of students in a tut
    int avl_slots;           // the availalble # of students in a tut
    int num_labs;            // no. of labs it can take mentors from
    int labs_list[MAX_LABS]; // list of the labs
    int curr_ta;             // stores the id of the current ta member, -1 if none
    int curr_lab;            // stores the lab from which current ta comes, -1 if none
    int is_available;        // 0 if course removed, 1 if available
    pthread_cond_t is_ready, tut_over;
};

// global variables
int num_students_g, num_labs_g, num_courses_g;
int tut_over_arr[MAX_COURSES], course_ready_arr[MAX_COURSES];
struct Lab all_labs[MAX_LABS];
struct Student students[MAX_STUDENTS];
struct Course courses[MAX_COURSES];
pthread_mutex_t labs_mutex[MAX_LABS], students_mutex[MAX_STUDENTS];
pthread_mutex_t courses_mutex[MAX_COURSES];

void *handle_student(void *args);
void *handle_course(void *args);
void destroy_all_mutex();
void remove_course(int c_id);
void relinquish_TA(int c_id);

void destroy_all_mutex()
{
    for (int i = 0; i < num_courses_g; i++)
    {
        pthread_mutex_destroy(&courses_mutex[i]);
    }
    for (int i = 0; i < num_students_g; i++)
    {
        pthread_mutex_destroy(&students_mutex[i]);
    }
    for (int i = 0; i < num_labs_g; i++)
    {
        pthread_mutex_destroy(&labs_mutex[i]);
        for (int j = 0; j < all_labs[i].num_students; j++)
            pthread_mutex_destroy(&all_labs[i].ta_mutex[j]);
    }

    return;
}

void *handle_student(void *args)
{
    int stu_id = ((struct Student *)args)->id;

    // sleep for t seconds
    sleep(students[stu_id].arrival_time);
    printf(BCYN "Student %d has filled in preferences for course registration\n" ANSI_RESET, stu_id);

    int cur_pref = students[stu_id].curr_pref;
    while (cur_pref < NUM_PREFS)
    {
        // loop for the preferences
        int c_id = students[stu_id].preferences[cur_pref];
        int c_id_next, change_flag = 0;
        if (cur_pref < 2)
            c_id_next = students[stu_id].preferences[cur_pref + 1];

        // printf("Student %d course %s pref %d\n", stu_id, courses[c_id].name, cur_pref + 1);

        while (1)
        {
            pthread_mutex_lock(&courses_mutex[c_id]);
            while (courses[c_id].is_available && !course_ready_arr[c_id])
            {
                // conditional wait for ta allotment
                pthread_cond_wait(&courses[c_id].is_ready, &courses_mutex[c_id]);
            }
            if ((courses[c_id].is_available != 1) || (courses[c_id].id == -1))
            {
                // checking for course availability, if id is -1 or not
                // if course not there skip to next preference
                pthread_mutex_unlock(&courses_mutex[c_id]);
                change_flag = 1;
                break;
            }

            // alloting to tut
            if ((course_ready_arr[c_id] == 1) && courses[c_id].avl_slots)
            {
                change_flag = 0;
                courses[c_id].avl_slots--;
                printf(BYEL "Student %d has been allocated a seat in course %s\n" ANSI_RESET,
                       stu_id, courses[c_id].name);
                pthread_mutex_unlock(&courses_mutex[c_id]);
                break;
            }
            pthread_mutex_unlock(&courses_mutex[c_id]);
        }

        if (change_flag == 1)
        {
        change_is_inevitable:
            if (cur_pref == 2)
            {
                printf(BRED "Student %d couldn’t get any of his preferred courses\n" ANSI_RESET, stu_id);
                students[stu_id].id = -1;
                return NULL;
            }
            printf(BBLU "Student %d has changed current preference from %s (priority %d) to %s (priority %d)\n" ANSI_RESET,
                   stu_id, courses[c_id].name, cur_pref + 1, courses[c_id_next].name, cur_pref + 2);
            cur_pref++;
            continue;
        }

        pthread_mutex_lock(&courses_mutex[c_id]);
        // conditional wait for finishing tut
        while (tut_over_arr[c_id] == 0)
        {
            pthread_cond_wait(&courses[c_id].tut_over, &courses_mutex[c_id]);
        }
        pthread_mutex_unlock(&courses_mutex[c_id]);

        // select whether to withdraw or not based on prob
        float prob = students[stu_id].calibre * courses[c_id].interest;
        if (prob > PROB)
        {
            // select the course
            printf(BGRN "Student %d has selected course %s permanently\n" ANSI_RESET,
                   stu_id, courses[c_id].name);
            return NULL;
        }
        else
        {
            students[stu_id].curr_pref++;
            printf(BRED "Student %d has withdrawn from course %s\n" ANSI_RESET,
                   stu_id, courses[c_id].name);
            goto change_is_inevitable;
        }
        cur_pref++;
    }

    // if no subj liked, exit the simulation, id=-1
    printf(BMAG "Student %d couldn’t get any of his preferred courses\n" ANSI_RESET, stu_id);
    students[stu_id].id = -1;

    return NULL;
}

void *handle_course(void *args)
{
    int c_id = ((struct Course *)args)->id;
    int lab_counter = 0;
    int num_labs = courses[c_id].num_labs;

    // infinite loop
    while (1)
    {
        lab_counter = 0;

        // loop for all labs
        for (int lab_i = 0; lab_i < num_labs; lab_i++)
        {
            int lab_idx = courses[c_id].labs_list[lab_i];

            pthread_mutex_lock(&labs_mutex[lab_idx]); // lock lab
            if (all_labs[lab_idx].id == -1)           // check if lab available
            {
                lab_counter++;
                pthread_mutex_unlock(&labs_mutex[lab_idx]);
                continue;
            }
            pthread_mutex_unlock(&labs_mutex[lab_idx]); // unlock lab

            int ta_counter = 0, ta_found = 0;
            // loop for all tas
            for (int ta_i = 0; ta_i < all_labs[lab_idx].num_students; ta_i++)
            {
                // lock TA
                pthread_mutex_lock(&all_labs[lab_idx].ta_mutex[ta_i]);

                // check if mentor is free and slots under limit
                if (all_labs[lab_idx].ta_limit_array[ta_i] < 1)
                {
                    ta_counter++;
                    pthread_mutex_unlock(&all_labs[lab_idx].ta_mutex[ta_i]);
                    continue;
                }
                else if (all_labs[lab_idx].ta_occupied_array[ta_i])
                {
                    pthread_mutex_unlock(&all_labs[lab_idx].ta_mutex[ta_i]);
                    continue;
                }

                // if yes, select the TA
                ta_found = 1;
                all_labs[lab_idx].ta_limit_array[ta_i]--;
                all_labs[lab_idx].ta_occupied_array[ta_i] = 1;
                courses[c_id].curr_ta = ta_i;
                courses[c_id].curr_lab = lab_idx;
                printf(BCYN "TA %d from lab %s has been allocated to course %s for %d TA ship\n" ANSI_RESET,
                       ta_i, all_labs[lab_idx].name, courses[c_id].name,
                       all_labs[lab_idx].ta_limit - all_labs[lab_idx].ta_limit_array[ta_i]);

                // unlock TA
                pthread_mutex_unlock(&all_labs[lab_idx].ta_mutex[ta_i]);
                break;
            }

            if (ta_found)
                break;

            // if all the TA have crossed the slots
            if (ta_counter == all_labs[lab_idx].num_students)
            {
                // lock lab
                pthread_mutex_lock(&labs_mutex[lab_idx]);
                // remove the lab, is_available = 0
                all_labs[lab_idx].id = -1;
                printf(BMAG "Lab %s no longer has students available for TA ship\n" ANSI_RESET,
                       all_labs[lab_idx].name);

                // unlock lab
                pthread_mutex_unlock(&labs_mutex[lab_idx]);
            }
        }

        if (lab_counter == num_labs)
            break;

        //  signal the student about course
        pthread_mutex_lock(&courses_mutex[c_id]);
        course_ready_arr[c_id] = 1;
        pthread_cond_broadcast(&courses[c_id].is_ready);
        pthread_mutex_unlock(&courses_mutex[c_id]);

        // choose a limit for selected students
        pthread_mutex_lock(&courses_mutex[c_id]);
        int stu_lim = rand() % courses[c_id].max_slots + 1;
        courses[c_id].avl_slots = stu_lim;
        printf(BYEL "Course %s has been allocated %d seats\n" ANSI_RESET, courses[c_id].name, stu_lim);
        pthread_mutex_unlock(&courses_mutex[c_id]);

        // wait till students come
        sleep(3);
        printf(BBLU "Tutorial has started for Course %s with %d seats filled out of %d \n" ANSI_RESET,
               courses[c_id].name, stu_lim - courses[c_id].avl_slots, stu_lim);

        pthread_mutex_lock(&courses_mutex[c_id]);
        // run the tute, a small delay for that
        course_ready_arr[c_id] = 0;
        sleep(3);
        relinquish_TA(c_id);
        tut_over_arr[c_id] = 1;
        pthread_cond_broadcast(&courses[c_id].tut_over);
        pthread_mutex_unlock(&courses_mutex[c_id]);

        sleep(1);
        pthread_mutex_lock(&courses_mutex[c_id]);
        tut_over_arr[c_id] = 0;
        pthread_mutex_unlock(&courses_mutex[c_id]);
    }

    // if no labs availabe
    if (lab_counter == num_labs)
        remove_course(c_id);

    return NULL;
}

void remove_course(int c_id)
{
    pthread_mutex_lock(&courses_mutex[c_id]);
    // remove the course
    courses[c_id].id = -1;
    courses[c_id].is_available = 0;
    printf(BRED "Course %s doesn’t have any TA’s eligible and is removed from course offering\n" ANSI_RESET,
           courses[c_id].name);

    //  signal the student about course
    course_ready_arr[c_id] = 1;
    pthread_cond_broadcast(&courses[c_id].is_ready);
    pthread_mutex_unlock(&courses_mutex[c_id]);
}

void relinquish_TA(int c_id)
{
    int clab = courses[c_id].curr_lab;

    pthread_mutex_lock(&labs_mutex[clab]);
    printf(BCYN "TA %d from lab %s has completed the tutorial and left the course %s\n" ANSI_RESET,
           courses[c_id].curr_ta, all_labs[clab].name, courses[c_id].name);
    all_labs[clab].ta_occupied_array[courses[c_id].curr_ta] = 0;
    pthread_mutex_unlock(&labs_mutex[clab]);
}

int main()
{
    srand(time(NULL));
    scanf("%d %d %d", &num_students_g, &num_labs_g, &num_courses_g);

    for (int i = 0; i < num_courses_g; i++)
    {
        char temp_str[MAX_STRING];
        int max_slots, num_labs;
        float interest;
        scanf("%s %f %d %d", temp_str, &interest, &max_slots, &num_labs);
        strcpy(courses[i].name, temp_str);
        courses[i].id = i;
        courses[i].interest = interest;
        courses[i].max_slots = max_slots;
        courses[i].num_labs = num_labs;
        courses[i].is_available = 1;
        courses[i].curr_ta = -1;
        courses[i].curr_lab = -1;
        for (int j = 0; j < num_labs; j++)
        {
            int lab_id;
            scanf("%d", &lab_id);
            courses[i].labs_list[j] = lab_id;
        }
        pthread_cond_init(&courses[i].is_ready, NULL);
        pthread_cond_init(&courses[i].tut_over, NULL);
        tut_over_arr[i] = 0;
        course_ready_arr[i] = 0;

        // initiating the mutex locks
        if (pthread_mutex_init(&courses_mutex[i], NULL))
        {
            printf("\nmutex error: unable to initialize mutex lock");
            printf("\nSimulation Ending.\n");
            // destroy_all_mutex();
            // return 0;
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_students_g; i++)
    {
        float cal;
        int p1, p2, p3, t;
        scanf("%f %d %d %d %d", &cal, &p1, &p2, &p3, &t);
        students[i].id = i;
        students[i].arrival_time = t;
        students[i].curr_pref = 0;
        students[i].preferences[0] = p1;
        students[i].preferences[1] = p2;
        students[i].preferences[2] = p3;
        students[i].calibre = cal;
        pthread_cond_init(&students[i].alloted_tut, NULL);

        // initiating the mutex locks
        if (pthread_mutex_init(&students_mutex[i], NULL))
        {
            printf("\nmutex error: unable to initialize mutex lock");
            printf("\nSimulation Ending.\n");
            // destroy_all_mutex();
            // return 0;
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_labs_g; i++)
    {
        char temp_str[MAX_STRING];
        int num_stu, ta_lim;
        scanf("%s %d %d", temp_str, &num_stu, &ta_lim);
        strcpy(all_labs[i].name, temp_str);
        all_labs[i].id = i;
        all_labs[i].num_students = num_stu;
        all_labs[i].ta_limit = ta_lim;

        // initiating the mutex locks
        if (pthread_mutex_init(&labs_mutex[i], NULL))
        {
            printf("\nmutex error: unable to initialize mutex lock");
            printf("\nSimulation Ending.\n");
            // destroy_all_mutex();
            // return 0;
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < num_stu; j++)
        {
            all_labs[i].ta_limit_array[j] = ta_lim;
            all_labs[i].ta_occupied_array[j] = 0;

            // initiating the mutex locks
            if (pthread_mutex_init(&all_labs[i].ta_mutex[j], NULL))
            {
                printf("\nmutex error: unable to initialize mutex lock");
                printf("\nSimulation Ending.\n");
                // destroy_all_mutex();
                // return 0;
                exit(EXIT_FAILURE);
            }
        }
    }

    if (num_courses_g < 1)
    {
        // exit
        printf("\nNo Courses given.");
        printf("\nSimulation Ending.\n");
        destroy_all_mutex();
        return 0;
    }
    if (num_students_g < 1)
    {
        // exit
        printf("\nNo Students given.");
        printf("\nSimulation Ending.\n");
        destroy_all_mutex();
        return 0;
    }
    if (num_labs_g < 1)
    {
        // exit
        printf("\nNo labs given.");
        printf("\nSimulation Ending.\n");
        destroy_all_mutex();
        return 0;
    }

    printf("\nSimulation Starting...\n");

    // declare threads
    pthread_t t_students[MAX_STUDENTS], t_courses[MAX_COURSES];

    // create threads
    for (int i = 0; i < num_students_g; i++)
    {
        pthread_create(&t_students[i], NULL, handle_student, (void *)(&students[i]));
    }
    for (int i = 0; i < num_courses_g; i++)
    {
        pthread_create(&t_courses[i], NULL, handle_course, (void *)(&courses[i]));
    }

    // join threads
    for (int i = 0; i < num_students_g; i++)
        pthread_join(t_students[i], NULL);
    // for (int i = 0; i < num_courses; i++)
    //     pthread_join(t_courses[i], NULL);

    printf("\nSimulation Finished Successfully.\n");
    destroy_all_mutex();
    return 0;
}