#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "scheduler.h"

#define CNT_CPUS 1
#define BASE_USER_TIME 8
#define SPAN_PROCESS_ARRIVAL_TIME 150
#define SPAN_PROCESS_EXEC_TIME 30
#define SPAN_CNT_PROCESSES 16
#define SPAN_CNT_USERS 16
#define SPAN_USER_WEIGHT 100
#define TIME_QUANTUM 3

struct CPU cpus[CNT_CPUS];

struct Process* generate_processes(struct User* usr)
{
  struct Process* prev = NULL;
  struct Process* head = NULL;

  for (int i = 0; i < usr->cnt_processes; i++)
  {
    struct Process* proc = malloc(sizeof(struct Process));
    proc->uid = usr->uid;
    proc->exec_time = rand() % SPAN_PROCESS_EXEC_TIME + 1;
    proc->arrival_time = rand() % SPAN_PROCESS_ARRIVAL_TIME + 1;

    pid_t pid = fork();

    if (pid == 0)
    {
      printf("User ID: %d with process %d\n", usr->uid, getpid());
      pause();
      printf("Process %d running on CPU...\n", getpid());
      sleep(proc->exec_time);
      printf("Process %d finished\n", getpid());
      exit(0);
    } else if (pid > 0)
    {
      proc->pid = pid;
    } else {
      perror("fork failed");
      exit(1);
    }

    proc->prev = prev;

    if (prev != NULL)
      prev->next = proc;
    else head = proc;

    prev = proc;
  }

  prev->next = head;
  head->prev = prev;

  return head;
}

struct User* generate_users(struct CPU* cpu)
{
  struct User* prev = NULL;
  struct User* head = NULL;

  for (int i = 0; i < cpu->cnt_users; i++)
  {
    struct User* usr = malloc(sizeof(struct User));
    usr->uid = i;
    // user->username = "user" + i;
    usr->weight = (rand() % SPAN_USER_WEIGHT) / (float)SPAN_USER_WEIGHT;
    usr->allocated_time = BASE_USER_TIME * usr->weight;
    usr->total_user_time = 0;
    printf("User ID: %d has allocated time %.5f \n", usr->uid, usr->allocated_time);

    usr->cnt_processes_available = 0;
    usr->cnt_processes_incoming = rand() % SPAN_CNT_PROCESSES + 1;

    usr->current_available = NULL;
    usr->current_incoming = generate_processes(usr);

    usr->prev = prev;
    if (prev != NULL)
      prev->next = usr;
    else head = usr;

    prev = usr;
  }
  prev->next = head;
  head->prev = prev;

  return head;
}

// method for updating available processes
float update_available_processes(struct User* usr)
{
  struct Process* min_arrival_proc = usr->current_incoming;
  
  for (int i = 0; i < usr->cnt_processes; i++)
  {
    if (usr->current_incoming->arrival_time < min_arrival_proc->arrival_time)
    {
      min_arrival_proc = usr->current_incoming;
    }

    if (usr->current_incoming->arrival_time <= usr->total_user_time)
    {
      usr->current_incoming->prev->next = usr->current_incoming->next;
      usr->current_incoming->next->prev = usr->current_incoming->prev;

      struct Process* aux_next = usr->current_incoming->next;

      if (usr->current_available == NULL)
      {
        usr->current_available = usr->current_incoming;
        usr->current_available->prev = NULL;
        usr->current_available->next = NULL;

      } else
      {
        struct Process* aux = usr->current_available->next;

        if (aux == NULL)
        {
          // lista are doar 1 element
          usr->current_available->next = usr->current_incoming;
          usr->current_incoming->next = usr->current_available;
          usr->current_incoming->prev = usr->current_available;
          usr->current_available->prev = usr->current_incoming;
        } else {
          usr->current_available->next = usr->current_incoming;
          usr->current_incoming->prev = usr->current_available;
          usr->current_incoming->next = aux;
          aux->prev = usr->current_incoming;
        }
      }
      
      usr->current_incoming = aux_next;
    } else usr->current_incoming = usr->current_incoming->next;
  }

  if (usr->current_available == NULL)
  {
    return min_arrival_proc->arrival_time;
  } 
  return 0;
}

float rr_processes_scheduler(struct User* usr)
{
  printf("Processes for user: %d\n", usr->uid);
  float retval = update_available_processes(usr);

  if (!retval)
  {
    struct Process*& proc = usr->current_available;

    float available_time = usr->allocated_time;
    float time_quantum = TIME_QUANTUM;

    while (available_time > 0 && proc != NULL)
    {   
      // TODO
    }

    return usr->allocated_time - available_time;
  } else
  {
    float waited = min(retval, allocated_time);
    sleep(waited);
    allocated_time -= waited;
    if (allocated_time > 0)
    {
       return waited + rr_processes_scheduler(usr);
    } 
    return waited;
  }    
}

float wrr_users_scheduler(struct CPU* cpu)
{
  // checking if this works =(
  struct User*& usr = cpu->current;

  while (usr != NULL)
  {
    usr->total_user_time += rr_processes_scheduler(usr);
  
    if (usr->cnt_processes_incoming == 0 && usr->cnt_process_available == 0)
    {
      struct User* aux = usr;
      usr->prev->next = usr->next;
      usr->next->prev = usr->prev;
      usr = usr->next;

      printf("User ID %d had a total time on CPU of %.5f \n", usr->uid, usr->total_user_time);
      free(aux);

    } else usr = usr->next;
  }
}

int main() 
{
  srand(time(NULL));

  for (int i = 0; i < CNT_CPUS; i++)
  {
    // generam nr de useri
    cpus[i].cpu_id = i;
    cpus[i].cnt_users = rand() % SPAN_CNT_USERS + 1;
    cpus[i].current = generate_users(&cpus[i]);
  }
  
  return 0;
}
