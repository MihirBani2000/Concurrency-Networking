
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>
#include <error.h>
#include <errno.h>

#define COLOR_BLACK "\033[0;30m"
#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_MAGENTA "\033[0;35m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_WHITE "\033[0;37m"
#define COLOR_RESET "\033[0m"

#define COLOR_GREEN_BOLD "\033[1;32m"
#define COLOR_RED_BOLD "\033[1;31m"
#define TEXT_UNDERLINE "\033[4m"
#define TEXT_BOLD "\033[1m"

int zone_h_capacity, zone_a_capacity, zone_n_capacity;
int spectating_time_x;
int num_groups, num_goal_chances, spectator_count;
int home_goals, away_goals;

int num_people_in_groups[1000];
int remaining_people_in_groups[1000];
pthread_mutex_t remaining_people_grps_lock[1000];
int zone_h_remaining, zone_a_remaining, zone_n_remaining;

pthread_t person_th[1000];
pthread_t goal_th[1000];
pthread_mutex_t home_lock, away_lock, zone_h_mutex, zone_a_mutex, zone_n_mutex;
pthread_cond_t home_cond, away_cond;
sem_t home_semaphore, away_semaphore, neutral_semaphore;
sem_t home_cond_semaphore, away_cond_semaphore, neutral_cond_semaphore;

struct Person
{
    int id;
    char name[1000];
    int away_home_neutral; //-1 is away, 0 neutral, 1 if home
    double patience_time;
    int enrage_goal_count;
    int group_id;
    double wait_time;
    bool finished_watching;
};

struct Person *persons[1000];

struct Goal
{
    int id;
    int team; //0 if HOME 1 if AWAY
    int goal_time;
    double probability;
};

struct Goal *goals[1000];

void *person_thread_function(void *arg)
{
    struct Person *person = (struct Person *)arg;
    sleep(person->wait_time);

    printf(COLOR_RED "%s has reached the stadium\n" COLOR_RESET, person->name);

    /* Calculate relative interval as current time plus 10 seconds */

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        /* handle error */
        return NULL;
    }

    ts.tv_sec += person->patience_time;

    bool completed = false;
    sem_t *selected_semaphore;
    int selected_zone; // -1 if A, 0 if N, 1 if H
    while (!completed)
    {
        if (person->away_home_neutral == -1)
        {
            int ret = sem_trywait(&away_semaphore);
            if (ret == 0)
            {
                printf(COLOR_GREEN "%s has got a seat in zone A\n" COLOR_RESET, person->name);
                completed = true;
                selected_semaphore = &away_semaphore;
                selected_zone = -1;
                continue;
            }
        }
        else if (person->away_home_neutral == 0)
        {
            int ret = sem_trywait(&neutral_semaphore);
            if (ret == 0)
            {
                printf(COLOR_GREEN "%s has got a seat in zone N\n" COLOR_RESET, person->name);
                completed = true;
                selected_semaphore = &neutral_semaphore;
                selected_zone = 0;
                continue;
            }
            ret = sem_trywait(&home_semaphore);
            if (ret == 0)
            {
                printf(COLOR_GREEN "%s has got a seat in zone H\n" COLOR_RESET, person->name);
                completed = true;
                selected_semaphore = &home_semaphore;
                selected_zone = 1;
                continue;
            }
            ret = sem_trywait(&away_semaphore);
            if (ret == 0)
            {
                printf(COLOR_GREEN "%s has got a seat in zone A\n" COLOR_RESET, person->name);
                completed = true;
                selected_semaphore = &away_semaphore;
                selected_zone = -1;
                continue;
            }
        }
        else if (person->away_home_neutral == 1)
        {
            int ret = sem_trywait(&neutral_semaphore);
            if (ret == 0)
            {
                printf(COLOR_GREEN "%s has got a seat in zone N\n" COLOR_RESET, person->name);
                completed = true;
                selected_semaphore = &neutral_semaphore;
                selected_zone = 0;
                continue;
            }
            ret = sem_trywait(&home_semaphore);
            if (ret == 0)
            {
                printf(COLOR_GREEN "%s has got a seat in zone H\n" COLOR_RESET, person->name);
                completed = true;
                selected_semaphore = &home_semaphore;
                selected_zone = 1;
                continue;
            }
        }
        // pthread_cond_t *waiting_cond;
        // pthread_mutex_t *dummy_mutex;
        sem_t *waiting_cond_semaphore;
        if (person->away_home_neutral == -1)
            waiting_cond_semaphore = &away_cond_semaphore;
        else if (person->away_home_neutral == 0)
            waiting_cond_semaphore = &neutral_cond_semaphore;
        else if (person->away_home_neutral == 1)
            waiting_cond_semaphore = &home_cond_semaphore;

        // pthread_mutex_lock(dummy_mutex);
        // int ret = pthread_cond_timedwait(waiting_cond, dummy_mutex, &ts);
        // pthread_mutex_unlock(dummy_mutex);
        int ret;
        while ((ret = sem_timedwait(waiting_cond_semaphore, &ts)) == -1 && errno == EINTR)
            continue; /* Restart if interrupted by handler */
        /* Check what happened */
        if (ret == -1)
        {
            if (errno == ETIMEDOUT)
            {
                printf(COLOR_MAGENTA "%s couldn't get a seat\n" COLOR_RESET, person->name);
                printf(COLOR_CYAN TEXT_UNDERLINE "%s is waiting for their friends at the exit\n" COLOR_RESET, person->name);
                pthread_mutex_lock(&remaining_people_grps_lock[person->group_id]);
                remaining_people_in_groups[person->group_id]--;
                if (remaining_people_in_groups[person->group_id] == 0)
                    printf(COLOR_YELLOW "Group %d is leaving for dinner\n" COLOR_RESET, person->group_id);
                pthread_mutex_unlock(&remaining_people_grps_lock[person->group_id]);
                return NULL;
            }
            else
            {
                perror("sem_timedwait Error");
            }
        }
    }
    // ========================================

    if (person->away_home_neutral == 0)
    {
        sleep(spectating_time_x);
        printf(TEXT_UNDERLINE COLOR_GREEN_BOLD "%s watched the match for %d seconds and is leaving\n" COLOR_RESET, person->name, spectating_time_x);
        sem_post(selected_semaphore);
        if (selected_zone == -1)
        {
            sem_post(&away_cond_semaphore);
            sem_post(&neutral_cond_semaphore);
        }
        else if (selected_zone == 0)
        {
            sem_post(&neutral_cond_semaphore);
            sem_post(&home_cond_semaphore);
        }
        else
        {
            sem_post(&neutral_cond_semaphore);
            sem_post(&home_cond_semaphore);
        }
        printf(COLOR_CYAN TEXT_UNDERLINE "%s is waiting for their friends at the exit\n" COLOR_RESET, person->name);
        pthread_mutex_lock(&remaining_people_grps_lock[person->group_id]);
        remaining_people_in_groups[person->group_id]--;
        if (remaining_people_in_groups[person->group_id] == 0)
            printf(COLOR_YELLOW "Group %d is leaving for dinner\n" COLOR_RESET, person->group_id);
        pthread_mutex_unlock(&remaining_people_grps_lock[person->group_id]);
        // printf(COLOR_YELLOW"%s is leaving for dinner\n"COLOR_RESET, person->name);
        return NULL;
    }

    double remaining_time = spectating_time_x;

    bool fan_not_enraged = true;

    clock_t og_time = clock();
    clock_t start_time = og_time;
    pthread_mutex_t *opponent_lock;
    pthread_cond_t *opponent_cond;
    int *opponent_goals;
    if (person->away_home_neutral == -1)
        opponent_cond = &home_cond, opponent_lock = &home_lock, opponent_goals = &home_goals;
    else
        opponent_cond = &away_cond, opponent_lock = &away_lock, opponent_goals = &away_goals;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        /* handle error */
        perror("CLOCK ERROR");
        return NULL;
    }

    ts.tv_sec += spectating_time_x;
    int ret_val;

    pthread_mutex_lock(opponent_lock);
    while (*opponent_goals < person->enrage_goal_count && !person->finished_watching)
    {
        while ((ret_val = pthread_cond_timedwait(opponent_cond, opponent_lock, &ts)) == -1)
            continue; /* Restart if interrupted by handler */
        /* Check what happened */
        if (ret_val != 0)
        {
            printf(TEXT_UNDERLINE COLOR_GREEN_BOLD "%s watched the match for %d seconds and is now leaving\n" COLOR_RESET, person->name, spectating_time_x);
            person->finished_watching = true;
        }
    }
    if (!person->finished_watching && (*opponent_goals) >= person->enrage_goal_count)
        printf(COLOR_GREEN_BOLD TEXT_UNDERLINE "%s is leaving due to bad performance of his team\n" COLOR_RESET, person->name);
    pthread_mutex_unlock(opponent_lock);

    sem_post(selected_semaphore);
    if (selected_zone == -1)
    {
        sem_post(&away_cond_semaphore);
        sem_post(&neutral_cond_semaphore);
    }
    else if (selected_zone == 0)
    {
        sem_post(&neutral_cond_semaphore);
        sem_post(&home_cond_semaphore);
    }
    else
    {
        sem_post(&neutral_cond_semaphore);
        sem_post(&home_cond_semaphore);
    }

    printf(COLOR_CYAN TEXT_UNDERLINE "%s is waiting for their friends at the exit\n" COLOR_RESET, person->name);
    pthread_mutex_lock(&remaining_people_grps_lock[person->group_id]);
    remaining_people_in_groups[person->group_id]--;
    if (remaining_people_in_groups[person->group_id] == 0)
        printf(COLOR_YELLOW "Group %d is leaving for dinner\n" COLOR_RESET, person->group_id);
    pthread_mutex_unlock(&remaining_people_grps_lock[person->group_id]);
    // printf(COLOR_YELLOW"%s is leaving for dinner\n"COLOR_RESET, person->name);
    return NULL;
}

void *goal_thread_function(void *arg)
{
    struct Goal *goal = (struct Goal *)arg;
    sleep(goal->goal_time);
    pthread_mutex_t *curr_team_lock;
    pthread_cond_t *curr_team_cond;
    char team_name;
    int *curr_team_goals;
    if (goal->team == 0)
        curr_team_cond = &home_cond, curr_team_lock = &home_lock, curr_team_goals = &home_goals, team_name = 'H';
    else
        curr_team_cond = &away_cond, curr_team_lock = &away_lock, curr_team_goals = &away_goals, team_name = 'A';

    double prob = goal->probability;
    double random_prob = (double)rand() / (double)RAND_MAX;

    if (random_prob < prob)
    {
        pthread_mutex_lock(curr_team_lock);
        (*curr_team_goals)++;
        printf(TEXT_UNDERLINE COLOR_BLUE "Team %c have scored their %dth goal\n" COLOR_RESET, team_name, *curr_team_goals);
        pthread_cond_broadcast(curr_team_cond);
        pthread_mutex_unlock(curr_team_lock);
    }
    else
    {
        pthread_mutex_lock(curr_team_lock);
        printf(TEXT_UNDERLINE COLOR_CYAN "Team %c missed the chance to score their %dth goal\n" COLOR_RESET, team_name, *curr_team_goals + 1);
        pthread_mutex_unlock(curr_team_lock);
    }
    return NULL;
}

int main()
{
    scanf("%d %d %d", &zone_h_capacity, &zone_a_capacity, &zone_n_capacity);

    zone_h_remaining = zone_h_capacity;
    zone_a_remaining = zone_a_capacity;
    zone_n_remaining = zone_n_capacity;

    home_goals = 0;
    away_goals = 0;

    pthread_mutex_init(&home_lock, NULL);
    pthread_mutex_init(&away_lock, NULL);
    pthread_mutex_init(&zone_h_mutex, NULL);
    pthread_mutex_init(&zone_a_mutex, NULL);
    pthread_mutex_init(&zone_n_mutex, NULL);
    pthread_cond_init(&home_cond, NULL);
    pthread_cond_init(&away_cond, NULL);
    sem_init(&away_cond_semaphore, 0, 0);
    sem_init(&home_cond_semaphore, 0, 0);
    sem_init(&neutral_cond_semaphore, 0, 0);

    scanf("%d", &spectating_time_x);
    scanf("%d", &num_groups);

    int y = 0;
    for (int x = 0; x < num_groups; x++)
    {
        scanf("%d", &num_people_in_groups[x]);
        remaining_people_in_groups[x] = num_people_in_groups[x];
        pthread_mutex_init(&remaining_people_grps_lock[x], NULL);
        for (int z = 0; z < num_people_in_groups[x]; z++)
        {
            persons[y] = calloc(1, sizeof(struct Person));
            persons[y]->id = y;
            char c;
            scanf("%s %c %lf %lf %d", persons[y]->name, &c, &persons[y]->wait_time, &persons[y]->patience_time, &persons[y]->enrage_goal_count);
            if (c == 'H')
                persons[y]->away_home_neutral = 1;
            else if (c == 'N')
                persons[y]->away_home_neutral = 0;
            else if (c == 'A')
                persons[y]->away_home_neutral = -1;
            persons[y]->group_id = x;
            y++;
            persons[x]->finished_watching = false;
            spectator_count++;
        }
    }
    scanf("%d", &num_goal_chances);
    for (int x = 0; x < num_goal_chances; x++)
    {
        char c;
        goals[x] = calloc(1, sizeof(struct Goal));
        scanf(" %c %d %lf", &c, &goals[x]->goal_time, &goals[x]->probability);
        if (c == 'H')
            goals[x]->team = 0;
        else
            goals[x]->team = 1;
    }

    // for(int x=0;x<num_goal_chances;x++)
    // {
    //     printf("=================\n");
    //     printf("%d %d %lf\n", goals[x]->team, goals[x]->goal_time, goals[x]->probability);
    //     printf("=========================\n");
    // }

    sem_init(&home_semaphore, 0, zone_h_capacity);
    sem_init(&away_semaphore, 0, zone_a_capacity);
    sem_init(&neutral_semaphore, 0, zone_n_capacity);

    for (int x = 0; x < spectator_count; x++)
    {
        pthread_create(&person_th[x], NULL, person_thread_function, persons[x]);
    }

    for (int x = 0; x < num_goal_chances; x++)
    {
        pthread_create(&goal_th[x], NULL, goal_thread_function, goals[x]);
    }

    for (int x = 0; x < spectator_count; x++)
    {
        pthread_join(person_th[x], NULL);
    }

    printf("SIMULATION OVER\n");
    exit(0);
}
