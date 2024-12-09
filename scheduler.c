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
    printf("User ID: %d has allocated time %.5f \n", usr->uid, usr->allocated_time);

    usr->cnt_processes = rand() % SPAN_CNT_PROCESSES + 1;
    
    usr->current = generate_processes(usr);

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
