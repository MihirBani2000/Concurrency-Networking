#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <error.h>
#include <errno.h>

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
#define MAX_PEOPLE 200
#define MAX_GROUPS 200
#define MAX_GOALS 200
#define MAX_STRING 255
#define PROB 0.5
#define AWAY -1
#define HOME 1
#define NEUTRAL 0

// struct for student
struct Person
{
    char name[MAX_STRING];
    int id;              // -1 if not valid
    int grp_id;          // -1 if not valid
    int arrival_time;    // time after it comes
    int pref_side;       // stores the preference, away, neutral, home
    int goal_limit;      // the goal limit after which he/she will leave
    float time_patience; //
    float time_wait;     //
    int watched;         // 1 if finished watching, 0 if watching
};

// struct for lab
struct Goal
{
    int id;           // -1 if not valid
    int team;         // 1 home, -1 away
    char team_name;   // home, away
    float prob;       // prob of happening
    int time_of_goal; // time at which it occured
};

// global variables
// related to spectators
int num_grps_g, num_persons_g, spec_time_x;
int num_ppl_grps[MAX_GROUPS], rem_ppl_grps[MAX_GROUPS];
struct Person persons[MAX_PEOPLE];
pthread_mutex_t rem_ppl_grps_mutex[MAX_GROUPS];

// related to goals
struct Goal goals[MAX_GOALS];
int h_goals_cnt_g, a_goals_cnt_g, num_goal_chances_g;

// related to zones
int zone_h_lim_g, zone_a_lim_g, zone_n_lim_g;
int zone_h_left_g, zone_a_left_g, zone_n_left_g;

pthread_mutex_t zone_h_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t zone_a_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t zone_n_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t home_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t away_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t home_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t away_cond = PTHREAD_COND_INITIALIZER;
sem_t home_sem, away_sem, neutral_sem;
sem_t home_cond_sem, away_cond_sem, neutral_cond_sem;

// function declarations
void *handle_person(void *args);
void *handle_goal(void *args);
void exit_person(int p_id);

void *handle_person(void *args)
{
    int pid = ((struct Person *)args)->id;
    // printf("HI %d %s\n", pid, persons[pid].name);

    // sleeping till its time
    sleep(persons[pid].time_wait);

    printf(BYEL "%s has reached the stadium\n" ANSI_RESET, persons[pid].name);

    struct timespec p_clock;
    int sel_zone;
    sem_t *sel_sem, *waiting_cond_sem;

    if (clock_gettime(CLOCK_REALTIME, &p_clock) < 0)
    {
        perror("error in clock_gettime()");
        return NULL;
    }
    p_clock.tv_sec += persons[pid].time_patience;

    // loop for getting a seat
    while (1)
    {
        // printf("--");
        if (persons[pid].pref_side == AWAY)
        {
            if (sem_trywait(&away_sem) == 0)
            {
                sel_zone = AWAY;
                sel_sem = &away_sem;
                printf(BCYN "%s has got a seat in zone A\n" ANSI_RESET,
                       persons[pid].name);
                break;
            }
        }
        else if (persons[pid].pref_side == HOME)
        {
            if (sem_trywait(&home_sem) == 0)
            {
                sel_zone = HOME;
                sel_sem = &home_sem;
                printf(BCYN "%s has got a seat in zone H\n" ANSI_RESET,
                       persons[pid].name);
                break;
            }
            if (sem_trywait(&neutral_sem) == 0)
            {
                sel_zone = NEUTRAL;
                sel_sem = &neutral_sem;
                printf(BCYN "%s has got a seat in zone N\n" ANSI_RESET,
                       persons[pid].name);
                break;
            }
        }
        else
        {
            if (sem_trywait(&neutral_sem) == 0)
            {
                sel_zone = NEUTRAL;
                sel_sem = &neutral_sem;
                printf(BCYN "%s has got a seat in zone N\n" ANSI_RESET,
                       persons[pid].name);
                break;
            }
            if (sem_trywait(&home_sem) == 0)
            {
                sel_zone = HOME;
                sel_sem = &home_sem;
                printf(BCYN "%s has got a seat in zone H\n" ANSI_RESET,
                       persons[pid].name);
                break;
            }
            if (sem_trywait(&away_sem) == 0)
            {
                sel_zone = AWAY;
                sel_sem = &away_sem;
                printf(BCYN "%s has got a seat in zone A\n" ANSI_RESET,
                       persons[pid].name);
                break;
            }
        }

        // waiting for a seat if not currently available
        if (persons[pid].pref_side == HOME)
        {
            waiting_cond_sem = &home_cond_sem;
        }
        else if (persons[pid].pref_side == AWAY)
        {
            waiting_cond_sem = &away_cond_sem;
        }
        else
        {
            waiting_cond_sem = &neutral_cond_sem;
        }

        int ret_val;
        while ((ret_val = sem_timedwait(waiting_cond_sem, &p_clock)) && (errno == EINTR))
        {
            // rerun the loop, as it is interrupted by handler
            continue;
        }
        if ((ret_val == -1) && (errno == ETIMEDOUT))
        {
            printf(BMAG "%s couldn’t get a seat\n" ANSI_RESET, persons[pid].name);

            // exiting and waiting for the grp to exit
            exit_person(pid);
            return NULL;
        }
        else if (ret_val == -1)
        {
            perror("error in sem_timedwait()");
        }
    }

    // now handling what happens after getting a seat

    // if neutral, dont need to check for goals and stuff
    if (persons[pid].pref_side == NEUTRAL)
    {
        // wait for spectating the match
        sleep(spec_time_x);
        printf(BMAG "%s watched the match for %d seconds and is leaving\n" ANSI_RESET,
               persons[pid].name, spec_time_x);

        // exiting
        goto exit_person_label;
    }

    pthread_cond_t *opp_cond;
    int *opp_goals;
    pthread_mutex_t *opp_mutex;

    if (persons[pid].pref_side == HOME)
    {
        opp_cond = &away_cond;
        opp_mutex = &away_mutex;
        opp_goals = &a_goals_cnt_g;
    }
    else
    {
        opp_cond = &home_cond;
        opp_mutex = &home_mutex;
        opp_goals = &h_goals_cnt_g;
    }

    if (clock_gettime(CLOCK_REALTIME, &p_clock) < 0)
    {
        perror("error in clock_gettime()");
        return NULL;
    }

    int ret;
    p_clock.tv_sec += spec_time_x;

    pthread_mutex_lock(opp_mutex);
    int goal_limit = persons[pid].goal_limit;
    while ((*opp_goals < goal_limit) && (persons[pid].watched == 0))
    {
        // ret = pthread_cond_timedwait(opp_cond, opp_mutex, &p_clock);
        while ((ret = pthread_cond_timedwait(opp_cond, opp_mutex, &p_clock)) == -1)
            ; // empty loop
        if (ret != 0)
        {
            // it was timed out
            printf(BMAG "%s watched the match for %d seconds and is now leaving\n" ANSI_RESET,
                   persons[pid].name, spec_time_x);
            persons[pid].watched = 1;
            pthread_mutex_unlock(opp_mutex);
            goto exit_person_label;
        }
    }

    if ((persons[pid].watched == 0) && (*opp_goals >= goal_limit))
    {
        printf(BMAG "%s is leaving due to bad performance of his team\n" ANSI_RESET,
               persons[pid].name);
    }
    pthread_mutex_unlock(opp_mutex);

exit_person_label:
    sem_post(sel_sem);
    // signal according to the zone
    if (sel_zone == AWAY)
    {
        sem_post(&away_cond_sem);
        sem_post(&neutral_cond_sem);
    }
    else
    {
        sem_post(&neutral_cond_sem);
        sem_post(&home_cond_sem);
    }
    exit_person(pid);
    return NULL;
}

void exit_person(int p_id)
{
    printf(BYEL "%s is waiting for their friends at the exit\n" ANSI_RESET,
           persons[p_id].name);

    // wait for others in the grp, to go together: BONUS
    int grp_id = persons[p_id].grp_id;
    pthread_mutex_lock(&rem_ppl_grps_mutex[grp_id]);
    if (--rem_ppl_grps[grp_id] == 0)
    {
        printf(BBLU "Group %d is leaving for dinner\n" ANSI_RESET, grp_id + 1);
    }
    pthread_mutex_unlock(&rem_ppl_grps_mutex[grp_id]);

    return;
}

void *handle_goal(void *args)
{
    int gid = ((struct Goal *)args)->id;

    // sleeping till its time
    sleep(goals[gid].time_of_goal);

    int *cur_team_goals;
    pthread_mutex_t *cur_team_mutex;
    pthread_cond_t *cur_team_cond;

    if (goals[gid].team == HOME)
    {
        cur_team_cond = &home_cond;
        cur_team_mutex = &home_mutex;
        cur_team_goals = &h_goals_cnt_g;
    }
    else
    {
        cur_team_cond = &away_cond;
        cur_team_mutex = &away_mutex;
        cur_team_goals = &a_goals_cnt_g;
    }

    float prob = (float)rand() / (float)RAND_MAX;
    pthread_mutex_lock(cur_team_mutex);
    if (prob > goals[gid].prob)
    {
        printf(BRED "Team %c missed the chance to score their %dth goal\n" ANSI_RESET,
               goals[gid].team_name, *cur_team_goals + 1);
    }
    else
    {
        printf(BGRN "Team %c have scored their %dth goal\n" ANSI_RESET,
               goals[gid].team_name, ++(*cur_team_goals));
        pthread_cond_broadcast(cur_team_cond);
    }
    pthread_mutex_unlock(cur_team_mutex);

    return NULL;
}

int main()
{
    srand(time(NULL));

    scanf("%d %d %d", &zone_h_lim_g, &zone_a_lim_g, &zone_n_lim_g);
    scanf("%d", &spec_time_x);
    scanf("%d", &num_grps_g);

    num_persons_g = 0;
    h_goals_cnt_g = 0, a_goals_cnt_g = 0;
    zone_h_left_g = zone_h_lim_g;
    zone_a_left_g = zone_a_lim_g;
    zone_n_left_g = zone_n_lim_g;

    sem_init(&away_cond_sem, 0, 0);
    sem_init(&home_cond_sem, 0, 0);
    sem_init(&neutral_cond_sem, 0, 0);
    sem_init(&home_sem, 0, zone_h_lim_g);
    sem_init(&away_sem, 0, zone_a_lim_g);
    sem_init(&neutral_sem, 0, zone_n_lim_g);

    int p_i = 0;
    for (int i = 0; i < num_grps_g; i++)
    {

        int num_ppl = 0;
        scanf("%d", &num_ppl);

        num_ppl_grps[i] = num_ppl;
        rem_ppl_grps[i] = num_ppl;
        num_persons_g += num_ppl;

        for (int j = 0; j < num_ppl; j++)
        {
            char ch;
            int goal_lim;
            float patience, wait_time;
            scanf("%s %c %f %f %d", persons[p_i].name, &ch, &wait_time,
                  &patience, &goal_lim);

            persons[p_i].id = p_i;
            persons[p_i].grp_id = i;
            persons[p_i].time_patience = patience;
            persons[p_i].time_wait = wait_time;
            persons[p_i].goal_limit = goal_lim;
            persons[p_i].watched = 0;
            if (ch == 'H')
                persons[p_i].pref_side = HOME;
            else if (ch == 'A')
                persons[p_i].pref_side = AWAY;
            else
                persons[p_i].pref_side = NEUTRAL;

            p_i++;
        }

        // initiating the mutex locks
        if (pthread_mutex_init(&rem_ppl_grps_mutex[i], NULL))
        {
            printf("\nmutex error: unable to initialize mutex lock");
            printf("\nSimulation Ending.\n");
            exit(EXIT_FAILURE);
        }
    }

    scanf("%d", &num_goal_chances_g);
    for (int i = 0; i < num_goal_chances_g; i++)
    {
        char ch;
        int gt;
        float prob;
        scanf(" %c %d %f", &ch, &gt, &prob);
        goals[i].id = i;
        goals[i].time_of_goal = gt;
        goals[i].prob = prob;
        goals[i].team = ch == 'H' ? HOME : AWAY;
        goals[i].team_name = ch;
        // printf("%c goal %d %c\n", ch, goals[i].team, goals[i].team_name);
    }

    printf("Simulation Starting...\n");

    // declare and create threads
    pthread_t t_person[num_persons_g];
    for (int i = 0; i < num_persons_g; i++)
        pthread_create(&t_person[i], NULL, handle_person, (void *)(&persons[i]));

    pthread_t t_goal[num_goal_chances_g];
    for (int i = 0; i < num_goal_chances_g; i++)
        pthread_create(&t_goal[i], NULL, handle_goal, (void *)(&goals[i]));

    // join threads
    for (int i = 0; i < num_persons_g; i++)
        pthread_join(t_person[i], NULL);

    printf("\nSimulation Finished Successfully.\n");
    return 0;
}
