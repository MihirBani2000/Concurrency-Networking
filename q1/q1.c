#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

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
    int id;                               // -1 if not valid
    int num_students;                     // students associated with it, to be TA mentors
    int ta_limit;                         // limit for each ta
    int ta_limit_array[MAX_MENTORS];      // stores the number of times left a mentor can become ta
    int ta_occupied_array[MAX_MENTORS];   // stores if mentor is busy
    pthread_mutex_t ta_mutex[MAX_MENTORS] // locks for each TA
};

// struct for course
struct Course
{
    char name[MAX_STRING];
    int id;                  // -1 if not valid
    float interest;          // the interest value of the course
    int max_slots;           // the max limit to be a TA
    int num_labs;            // no. of labs it can take mentors
    int labs_list[MAX_LABS]; // list of the labs
    int curr_ta;             // stores the id of the current ta member, -1 if none
    int curr_lab;            // stores the lab from which current ta comes, -1 if none
    int is_available;        // 0 if course removed, 1 if available
};

// global variables
int num_students_g, num_labs_g, num_courses_g;
struct Lab labs[MAX_LABS];
struct Student students[MAX_STUDENTS];
struct Course courses[MAX_COURSES];
pthread_mutex_t labs_mutex[MAX_LABS], students_mutex[MAX_STUDENTS];
pthread_mutex_t courses_mutex[MAX_COURSES];

void *handle_student(void *args);
void *handle_course(void *args);
// void *handle_lab(void *args);
void destroy_all_mutex();

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
    }

    return;
}

void *handle_student(void *args)
{
    int stu_id = ((struct Student *)args)->id;

    // sleep for t seconds
    sleep(students[stu_id].arrival_time);
    printf("Student %d has filled in preferences for course registration\n", stu_id);

    for (int pref_idx = 0; pref_idx < NUM_PREFS && students[stu_id].curr_pref < NUM_PREFS; pref_idx++)
    { // loop for the preferences
        int cur_pref = students[stu_id].curr_pref;
        int c_id = students[stu_id].preferences[cur_pref];
        int c_id_next;
        if (cur_pref < 2)
            c_id_next = students[stu_id].preferences[cur_pref + 1];

        pthread_mutex_lock(&courses[c_id]);
        if ((courses[c_id].is_available != 1) || (courses[c_id].id == -1))
        {
            // checking for course availability, if id is -1 or not
            // if course not there
            // skip to next preference
            pthread_mutex_unlock(&courses[c_id]);

            students[stu_id].curr_pref++;
            if (cur_pref < 2)
                printf("Student %d has changed current preference from %s (priority %d) to %s (priority %d)\n",
                       stu_id, courses[c_id].name, cur_pref + 1, courses[c_id_next].name, cur_pref + 2);
            else
            {
                printf("Student %d couldn’t get any of his preferred courses\n", stu_id);
                students[stu_id].id = -1;
                return NULL;
            }
            continue;
        }
        pthread_mutex_unlock(&courses[c_id]);

        // conditional wait for alloting to tut
        // conditional wait
        // conditional wait for finishing tut

        // select whether to withdraw or not based on prob
        float prob = students[stu_id].calibre * courses[c_id].interest;
        if (prob > PROB)
        {
            // select the course
            printf("Student %d has selected course %s permanently\n", stu_id, courses[c_id].name);
            return NULL;
        }
        else
        {
            students[stu_id].curr_pref++;
            printf("Student %d has withdrawn from course %s\n", stu_id, courses[c_id].name);
        }
    }

    // if no subj liked, exit the simulation, id=-1
    printf("Student %d couldn’t get any of his preferred courses\n", stu_id);
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
        // loop for all labs
        for (int lab_i = 0; lab_i < num_labs; lab_i++)
        {
            int lab_idx = courses[c_id].labs_list[lab_i];

            pthread_mutex_lock(&labs_mutex[lab_idx]); // lock lab
            if (labs[lab_idx].id == -1)               // check if lab available
            {
                lab_counter++;
                pthread_mutex_unlock(&labs_mutex[lab_idx]);
                continue;
            }
            pthread_mutex_unlock(&labs_mutex[lab_idx]); // unlock lab

            int ta_counter = 0;
            // loop for all tas
            for (int ta_i = 0; ta_i < labs[lab_idx].num_students; ta_i++)
            {
                // lock TA
                pthread_mutex_lock(&labs[lab_idx].ta_mutex[ta_i]);

                // check if mentor is free and slots under limit
                if (labs[lab_idx].ta_limit_array[ta_i] < 1)
                {
                    ta_counter++;
                    pthread_mutex_unlock(&labs[lab_idx].ta_mutex[ta_i]);
                    continue;
                }
                else if (labs[lab_idx].ta_occupied_array[ta_i])
                {
                    pthread_mutex_unlock(&labs[lab_idx].ta_mutex[ta_i]);
                    continue;
                }

                // if yes, select the TA
                labs[lab_idx].ta_limit_array[ta_i]--;
                labs[lab_idx].ta_occupied_array[ta_i] = 1;
                courses[c_id].curr_ta = ta_i;
                courses[c_id].curr_lab = lab_idx;
                printf("TA %d from lab %s has been allocated to course %s for %d TA ship\n",
                       ta_i, labs[lab_idx].name, courses[c_id].name,
                       labs[lab_idx].ta_limit - labs[lab_idx].ta_limit_array[ta_i]);

                // unlock TA
                pthread_mutex_unlock(&labs[lab_idx].ta_mutex[ta_i]);
            }
            // if all the TA have crossed the slots
            if (ta_counter == labs[lab_idx].num_students)
            {
                // lock lab
                pthread_mutex_lock(&labs_mutex[lab_idx]);
                // remove the lab, is_available = 0
                labs[lab_idx].id = -1;
                printf("Lab %s no longer has students available for TA ship\n", labs[lab_idx].name);

                // unlock lab
                pthread_mutex_unlock(&labs_mutex[lab_idx]);
            }
        }

        if (lab_counter == num_labs)
            break;
        else
            lab_counter = 0;
    }

    // if no labs availabe
    if (lab_counter == num_labs)
    {
        pthread_mutex_lock(&courses_mutex[c_id]);
        // remove the course
        courses[c_id].id = -1;
        courses[c_id].is_available = 0;
        printf("Course %s doesn’t have any TA’s eligible and is removed from course offering\n",
               courses[c_id].name);
        pthread_mutex_unlock(&courses_mutex[c_id]);

        //  signal the student about it

        return NULL;
    }

    // choose a limit for selected students
    // wait till students come, under the limit
    // conditional wait
    // run the tute, a small delay for that
    // relinquish the TA
    return NULL;
}

int main()
{
    scanf("%d %d %d", &num_students_g, &num_labs_g, &num_courses_g);

    for (int i = 0; i < num_courses_g; i++)
    {
        char temp_str[MAX_STRING];
        int interest, max_slots, num_labs;
        scanf("%s %f %d %d", temp_str, &interest, &max_slots, &num_labs);
        strcpy(courses[i].name, temp_str);
        courses[i].id = i;
        courses[i].interest = interest;
        courses[i].max_slots = max_slots;
        courses[i].num_labs = num_labs;
        for (int j = 0; j < num_labs; j++)
        {
            int lab_id;
            scanf("%d", lab_id);
            courses[i].labs_list[j] = lab_id;
        }

        // initiating the mutex locks
        if (pthread_mutex_init(&courses_mutex[i], NULL))
        {
            printf("\nmutex error: unable to initialize mutex lock");
            printf("\nSimulation Ending.\n");
            destroy_all_mutex();
            return 0;
        }
    }

    for (int i = 0; i < num_students_g; i++)
    {
        int cal, p1, p2, p3, t;
        scanf("%d %d %d %d %d", &cal, &p1, &p2, &p3, &t);
        students[i].id = i;
        students[i].calibre = cal;
        students[i].curr_pref = 0;
        students[i].preferences[0] = p1;
        students[i].preferences[1] = p2;
        students[i].preferences[2] = p3;
        students[i].arrival_time = t;

        // initiating the mutex locks
        if (pthread_mutex_init(&students_mutex[i], NULL))
        {
            printf("\nmutex error: unable to initialize mutex lock");
            printf("\nSimulation Ending.\n");
            destroy_all_mutex();
            return 0;
        }
    }

    for (int i = 0; i < num_labs_g; i++)
    {
        char temp_str[MAX_STRING];
        int num_stu, ta_lim;
        scanf("%s %d %d", temp_str, &num_stu, &ta_lim);
        strcpy(labs[i].name, temp_str);
        labs[i].id = i;
        labs[i].num_students = num_stu;
        labs[i].ta_limit = ta_lim;

        // initiating the mutex locks
        if (pthread_mutex_init(&labs_mutex[i], NULL))
        {
            printf("\nmutex error: unable to initialize mutex lock");
            printf("\nSimulation Ending.\n");
            destroy_all_mutex();
            return 0;
        }

        for (int j = 0; j < num_stu; j++)
        {
            labs[i].ta_limit_array[j] = ta_lim;
            labs[i].ta_occupied_array[j] = 0;

            // initiating the mutex locks
            if (pthread_mutex_init(&labs[i].ta_mutex[j], NULL))
            {
                printf("\nmutex error: unable to initialize mutex lock");
                printf("\nSimulation Ending.\n");
                destroy_all_mutex();
                return 0;
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