#include "GoalChance.h"

bool is_successful(GoalChance *gc)
{
    double rand_prob = (double)rand() / (double)RAND_MAX;

    return rand_prob <= gc->probability ? true : false;
}

GoalChance* new_goal_chance_from_input()
{
    GoalChance *goal_chance = malloc(sizeof(GoalChance));
    if (goal_chance == NULL)
        err_n_die("Failed to create a new GoalChance");

    scanf(" %c %lld %lf", &goal_chance->team, &goal_chance->time, &goal_chance->probability);

    goal_chance->is_successful = is_successful;

    return goal_chance;
}

void print_goal_chance(GoalChance *gc)
{
    printf("\n");

    printf(COLOR_BLUE "Goal Chance Team: " COLOR_YELLOW "%c" COLOR_RESET "\n", gc->team);
    printf(COLOR_BLUE "Goal Chance Time: " COLOR_YELLOW "%lld" COLOR_RESET "\n", gc->time);
    printf(COLOR_BLUE "Goal Chance Probability: " COLOR_YELLOW "%lf" COLOR_RESET "\n", gc->probability);

    printf("\n");
}
#ifndef __Q2_GOAL_CHANCE_H__
#define __Q2_GOAL_CHANCE_H__

#include "../common.h"

typedef struct GoalChance
{
    pthread_t thread;
    char team;
    llint time;
    double probability;

    bool (*is_successful)(struct GoalChance*);
}
GoalChance;

GoalChance* new_goal_chance_from_input();
void print_goal_chance(GoalChance *gc);

#endif
#include "Group.h"

Group* new_group_from_input(llint num_people)
{
    Group *group = malloc(sizeof(Group));
    if (group == NULL)
        err_n_die("Failed to create a new Group");

    group->num_people = num_people;
    group->people = malloc(num_people * sizeof(Person*));
    if (group->people == NULL)
        err_n_die("Failed to create Group->People");

    return group;
}
#ifndef __Q2_GROUP_H__
#define __Q2_GROUP_H__

#include "../common.h"
#include "Person.h"

typedef struct Group
{
    llint num_people;
    Person **people;
}
Group;

Group* new_group_from_input(llint num_people);

#endif
#include "Person.h"

Person* new_person_from_input(llint group_number)
{
    Person *person = malloc(sizeof(Person));
    if (person == NULL)
        err_n_die("Failed to create a new Person");

    person->group_number = group_number;

    // scanf("%s", person->name);
    // scanf("%c", &person->team);
    // scanf("%lld", &person->arrival_time_delay);
    // scanf("%lld", &person->patience_time);
    // scanf("%lld", &person->goal_threshold);

    // scanf("%s %c %lf %lf %d", persons[y]->name, &c, &persons[y]->wait_time, &persons[y]->patience_time, &persons[y]->enrage_goal_count);
    scanf("%s %c %lld %lld %lld", person->name, &person->team, &person->arrival_time_delay, &person->patience_time, &person->goal_threshold);

    person->finished_watching = false;
    person->zone = NULL;

    return person;
}

void print_person(Person *person)
{
    printf("\n");

    printf(COLOR_BLUE "Person Name: " COLOR_YELLOW "%s" COLOR_RESET "\n", person->name);
    printf(COLOR_BLUE "Person Group Number: " COLOR_YELLOW "%lld" COLOR_RESET "\n", person->group_number);
    printf(COLOR_BLUE "Person Team: " COLOR_YELLOW "%c" COLOR_RESET "\n", person->team);
    printf(COLOR_BLUE "Person Arrival Time: " COLOR_YELLOW "%lld" COLOR_RESET "\n", person->arrival_time_delay);
    printf(COLOR_BLUE "Person Patience Time: " COLOR_YELLOW "%lld" COLOR_RESET "\n", person->patience_time);
    printf(COLOR_BLUE "Person Goals Threshold: " COLOR_YELLOW "%lld" COLOR_RESET "\n", person->goal_threshold);

    printf("\n");
}
#ifndef __Q2_PERSON_H__
#define __Q2_PERSON_H__

#include "../common.h"
#include "Zone.h"

typedef struct Person
{
    pthread_t thread;
    char name[PERSON_NAME_LEN];
    char team;
    llint patience_time;
    llint group_number;
    llint arrival_time_delay;
    llint goal_threshold;
    Zone *zone;
    bool finished_watching;
}
Person;

Person* new_person_from_input(llint group_number);
void print_person(Person *person);

#endif
#include "Team.h"

Team* new_team(char name)
{
    Team *team = malloc(sizeof(Team));
    if (team == NULL)
        err_n_die("Failed to create a new team");

    team->name = name;
    team->goals = 0;
    pthread_mutex_init(&team->lock, NULL);
    pthread_cond_init(&team->goal_cond, NULL);

    return team;
}
#ifndef __Q2_TEAM_H__
#define __Q2_TEAM_H__

#include "../common.h"

typedef struct Team
{
    char name;
    llint goals;
    pthread_mutex_t lock;
    pthread_cond_t goal_cond;
}
Team;

Team* new_team(char name);

#endif
#include "Zone.h"

Zone* new_zone_from_input(char name)
{
    Zone *zone = malloc(sizeof(Zone));
    if(zone == NULL)
        err_n_die("Failed to create a Zone");

    scanf("%lld", &zone->capacity);

    sem_init(&zone->seats_left, 0, zone->capacity);
    zone->name = name;

    return zone;
}

void print_zone(Zone *zone)
{
    printf("\n");
    printf(COLOR_BLUE "Zone Name: " COLOR_YELLOW "%c" COLOR_RESET "\n", zone->name);
    printf(COLOR_BLUE "Zone Capacity: " COLOR_YELLOW "%lld" COLOR_RESET "\n", zone->capacity);
    printf("\n");
}
#ifndef __Q2_ZONE_H__
#define __Q2_ZONE_H__

#include "../common.h"

typedef struct Zone
{
    char name;
    llint capacity;                     /* Done */
    sem_t seats_left;                   /* Done */
}
Zone;

Zone* new_zone_from_input(char name);
void print_zone(Zone *zone);

#endif
#ifndef __Q2_COMMON_H
#define __Q2_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include "utils.h"

typedef long long int llint;

#define PERSON_NAME_LEN 50

#define HOME_TEAM_SUPPORTERS 0
#define AWAY_TEAM_SUPPORTERS 1
#define NEUTRAL_SUPPORTERS 2

#define DEBUG 1
#define INFO_LEVEL 0

// Ascii codes for 256-bit colors
#define COLOR_BLACK   "\033[0;30m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_YELLOW  "\033[0;33m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_MAGENTA "\033[0;35m"
#define COLOR_CYAN    "\033[0;36m"
#define COLOR_WHITE   "\033[0;37m"
#define COLOR_RESET   "\033[0m"

#define COLOR_GREEN_BOLD   "\033[1;32m"
#define COLOR_RED_BOLD     "\033[1;31m"
#define TEXT_UNDERLINE     "\033[4m"
#define TEXT_BOLD          "\033[1m"

#endif
#include "common.h"
#include "classes/Group.h"
#include "classes/Person.h"
#include "classes/Zone.h"
#include "classes/GoalChance.h"
#include "classes/Team.h"

llint spectating_time;
llint num_groups;
llint num_goal_chances;

Zone *zone_H, *zone_A, *zone_N;
Group **all_groups;
GoalChance **all_goal_chances;

Team *home_team, *away_team;

sem_t zone_seats_semaphores[3];

pthread_t goal_t;

#if DEBUG > 0
void print_seats()
{
    int sem_values_g_1;
    int sem_values_g_2;
    int sem_values_g_3;
    int sem_values_h;
    int sem_values_a;
    int sem_values_n;
    sem_getvalue(&zone_seats_semaphores[HOME_TEAM_SUPPORTERS], &sem_values_g_1);
    sem_getvalue(&zone_seats_semaphores[AWAY_TEAM_SUPPORTERS], &sem_values_g_2);
    sem_getvalue(&zone_seats_semaphores[NEUTRAL_SUPPORTERS], &sem_values_g_3);
    sem_getvalue(&zone_H->seats_left, &sem_values_h);
    sem_getvalue(&zone_A->seats_left, &sem_values_a);
    sem_getvalue(&zone_N->seats_left, &sem_values_n);

    printf(\
            "G_H: %d, G_A: %d, G_N: %d, "\
            "Z_H: %d, Z_A: %d, Z_N: %d\n",\
            sem_values_g_1,\
            sem_values_g_2,\
            sem_values_g_3,\
            sem_values_h,\
            sem_values_a,\
            sem_values_n\
          );
}
#endif

void* find_seats_thread(void *arg)
{

    return NULL;
}

void* people_thread(void *arg)
{
    Person *person = (Person*)arg;
    sleep(person->arrival_time_delay);

    printf(\
            COLOR_RED\
            "%s has reached the stadium"\
            COLOR_RESET"\n",\
            person->name\
          );

    sem_t *seats_semaphore = NULL;

    if (person->team == 'H')
        seats_semaphore = &zone_seats_semaphores[HOME_TEAM_SUPPORTERS];
    else if (person->team == 'A')
        seats_semaphore = &zone_seats_semaphores[AWAY_TEAM_SUPPORTERS];
    else
        seats_semaphore = &zone_seats_semaphores[NEUTRAL_SUPPORTERS];

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        err_n_die("Failed to get current time");

    llint rt;
    ts.tv_sec += person->patience_time;

    while (person->zone == NULL) {
        while ((rt = sem_timedwait(seats_semaphore, &ts)) != 0 && errno == EINTR)
            continue;

        if (rt != 0)
        {
            if (errno == ETIMEDOUT) {
                printf(\
                        COLOR_MAGENTA\
                        "%s could not get a seat"\
                        COLOR_RESET"\n",\
                        person->name\
                      );
                goto dinner_party;
            }
            else
                err_n_die("Failed to wait for a seat");
        }

        if (person->team == 'H')
        {
            if (sem_trywait(&zone_H->seats_left) == 0)
                person->zone = zone_H;
            else if (sem_trywait(&zone_N->seats_left) == 0)
                person->zone = zone_N;
        }
        else if (person->team == 'A')
        {
            sem_wait(&zone_A->seats_left);
            person->zone = zone_A;
        }
        else
        {
            if (sem_trywait(&zone_H->seats_left) == 0)
                person->zone = zone_H;
            else if (sem_trywait(&zone_A->seats_left) == 0)
                person->zone = zone_A;
            else if (sem_trywait(&zone_N->seats_left) == 0)
                person->zone = zone_N;
        }
    }
    if (person->zone == NULL) {
        printf("%s failed to get a seat even when he got a seat\n", person->name);
    }

    printf(\
            COLOR_MAGENTA\
            "%s has got a seat in zone %c"\
            COLOR_RESET"\n",\
            person->name,\
            person->zone->name\
          );

#if DEBUG > 0
    print_seats();
#endif

    if (person->team == 'N')
    {
        sleep(spectating_time);
        printf(\
                COLOR_GREEN\
                "%s watched the match for %lld seconds and is leaving"\
                COLOR_RESET"\n",\
                person->name,\
                spectating_time\
              );
    }

    else {
        Team *opponent_team = (person->team == 'H') ? away_team : home_team;
        pthread_mutex_t *goal_lock_p = &opponent_team->lock;
        pthread_cond_t *goal_cond_p = &opponent_team->goal_cond;

        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            err_n_die("Failed to get current time");

        ts.tv_sec += spectating_time;
        pthread_mutex_lock(goal_lock_p);

        while (opponent_team->goals < person->goal_threshold && !person->finished_watching)
        {
            rt = pthread_cond_timedwait(goal_cond_p, goal_lock_p, &ts);

            if (rt != 0){
                printf(\
                        COLOR_GREEN\
                        "%s watched the match for %lld seconds and is leaving"\
                        COLOR_RESET"\n",\
                        person->name,\
                        spectating_time\
                      );
                person->finished_watching = true;
            }

        }

        if (!person->finished_watching && opponent_team->goals >= person->goal_threshold)
        {
            printf(\
                    COLOR_GREEN\
                    "%s is leaving due to bad performance of his team"\
                    COLOR_RESET"\n",\
                    person->name\
                  );
        }

        pthread_mutex_unlock(&opponent_team->lock);
    }

    sem_post(&person->zone->seats_left);
    sem_post(seats_semaphore);

dinner_party:
    printf(COLOR_YELLOW "%s is leaving for dinner" COLOR_RESET "\n", person->name);

    return NULL;
}

// Goals simulation ke liye thread
void* goals_thread(void *arg)
{
    GoalChance *goal = (GoalChance*)arg;
    Team *team = goal->team == 'H' ? home_team : away_team;

#if DEBUG > 0
    printf("Goal Chance at time = %lld is starting\n", goal->time);
#endif

    sleep(goal->time);

    if (goal->is_successful(goal))
    {
        pthread_mutex_lock(&team->lock);
        team->goals += 1;
        printf(\
                TEXT_BOLD\
                "Team %c has scored their %lldth goal"\
                COLOR_RESET"\n",\
                team->name,\
                team->goals\
              );
        pthread_cond_broadcast(&team->goal_cond);
        pthread_mutex_unlock(&team->lock);
    }
    else
    {
        printf(\
                TEXT_BOLD\
                "Team %c has failed to score their %lldth goal"\
                COLOR_RESET"\n",\
                team->name,\
                team->goals+1\
              );
    }

#if DEBUG > 0
    printf("Goal Chance at time = %lld is exiting\n", goal->time);
#endif

    return NULL;
}

int main(int argc, char **argv)
{
    /* Take input */
    zone_H = new_zone_from_input('H');
    zone_A = new_zone_from_input('A');
    zone_N = new_zone_from_input('N');

    scanf("%lld", &spectating_time);
    scanf("%lld", &num_groups);

#if DEBUG > 0
    print_zone(zone_H);
    print_zone(zone_A);
    print_zone(zone_N);
#endif

    all_groups = malloc(num_groups * sizeof(Group*));

    for (llint grp_num=0; grp_num<num_groups; grp_num++)
    {
        llint num_people;
        scanf("%lld", &num_people);
        Group *group = new_group_from_input(num_people);
        all_groups[grp_num] = group;
        for (llint person_num=0; person_num<num_people; person_num++)
        {
            Person *person = new_person_from_input(grp_num);
#if DEBUG > 0
            print_person(person);
#endif
            group->people[person_num] = person;
        }
    }

    scanf("%lld", &num_goal_chances);

    all_goal_chances = malloc(num_goal_chances * sizeof(GoalChance*));

    for (llint goal_chance_num=0; goal_chance_num<num_goal_chances; goal_chance_num++)
    {
        all_goal_chances[goal_chance_num] = new_goal_chance_from_input();
#if DEBUG > 0
        print_goal_chance(all_goal_chances[goal_chance_num]);
#endif
    }

    away_team = new_team('A');
    home_team = new_team('H');

    sem_init(&zone_seats_semaphores[HOME_TEAM_SUPPORTERS], 0, zone_H->capacity + zone_N->capacity);
    sem_init(&zone_seats_semaphores[AWAY_TEAM_SUPPORTERS], 0, zone_A->capacity);
    sem_init(&zone_seats_semaphores[NEUTRAL_SUPPORTERS], 0, zone_H->capacity + zone_N->capacity + zone_A->capacity);

#if DEBUG > 0
    int sem_values;
    sem_getvalue(&zone_seats_semaphores[HOME_TEAM_SUPPORTERS], &sem_values);
    printf("HOME TEAM COMBINED SEATS: %d\n", sem_values);
    sem_getvalue(&zone_seats_semaphores[AWAY_TEAM_SUPPORTERS], &sem_values);
    printf("AWAY TEAM COMBINED SEATS: %d\n", sem_values);
    sem_getvalue(&zone_seats_semaphores[NEUTRAL_SUPPORTERS], &sem_values);
    printf("NEUTRAL TEAM COMBINED SEATS: %d\n", sem_values);
#endif

    for (llint group_num=0; group_num<num_groups; group_num++)
    {
        Group *group = all_groups[group_num];
        for (llint person_num=0; person_num<group->num_people; person_num++)
        {
            Person *person = group->people[person_num];
            pthread_create(&person->thread, NULL, people_thread, person);
        }
    }

    for (llint goal_chance_num=0; goal_chance_num<num_goal_chances; goal_chance_num++)
    {
        GoalChance *gc = all_goal_chances[goal_chance_num];
        pthread_create(&gc->thread, NULL, goals_thread, (void*)gc);
    }

    for (llint group_num=0; group_num<num_groups; group_num++)
    {
        Group *group = all_groups[group_num];
        for (llint person_num=0; person_num<group->num_people; person_num++)
        {
            Person *person = group->people[person_num];
            pthread_join(person->thread, NULL);
        }
    }

    for (llint goal_chance_num=0; goal_chance_num<num_goal_chances; goal_chance_num++)
    {
        GoalChance *gc = all_goal_chances[goal_chance_num];
        pthread_join(gc->thread, NULL);
    }

    return 0;
}
#include "common.h"
#include "utils.h"

void err_n_die(const char *fmt, ...)
{
    int errno_save;
    va_list ap;

    errno_save = errno;

    fprintf(stderr, COLOR_RED);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);

    if (errno_save != 0)
    {
        fprintf(stderr, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
        fprintf(stderr, "\n");
        fprintf(stderr, COLOR_RESET);
        fflush(stderr);
    }

    va_end(ap);
    exit(1);
}
#ifndef __Q2_UTILS_H
#define __Q2_UTILS_H

void err_n_die(const char *fmt, ...);

#endif
