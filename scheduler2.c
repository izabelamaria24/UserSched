#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#include "scheduler2.h"

#define CNT_CPUS 1
#define BASE_USER_TIME 8
#define SPAN_PROCESS_ARRIVAL_TIME 50
#define SPAN_PROCESS_EXEC_TIME 30
#define SPAN_CNT_PROCESSES 16
#define SPAN_CNT_USERS 16
#define SPAN_USER_WEIGHT 100
#define TIME_QUANTUM 3

CPU cpus[CNT_CPUS];


static inline float min(float a, float b) {
    return (a < b) ? a : b;
}


Process* generate_processes(User* usr)
{
  Process* prev = NULL;
  Process* head = NULL;

  for (int i = 0; i < usr->cnt_processes_incoming; i++)
  {
    Process* proc = malloc(sizeof(Process));
    proc->uid = usr->uid;
    proc->exec_time = rand() % SPAN_PROCESS_EXEC_TIME + 1;
    proc->arrival_time = rand() % SPAN_PROCESS_ARRIVAL_TIME + 1;
    //printf("Exec %d, Arrival %d\n",proc->exec_time, proc->arrival_time);
    pid_t pid = fork();

    if (pid == 0)
    {
      printf("User ID: %d with process %d and arrival time %f\n", usr->uid, getpid(), proc->arrival_time);
      kill(getpid(), SIGSTOP);
      while(true){
        sleep(1);
        printf("Process %d running on CPU...\n", getpid());

      }
  
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

User* generate_users(CPU* cpu)
{
  User* prev = NULL;
  User* head = NULL;

  for (int i = 0; i < cpu->cnt_users; i++)
  {
    User* usr = malloc(sizeof(User));
    usr->uid = i;
    // user->username = "user" + i;
    usr->weight = (rand() % SPAN_USER_WEIGHT) / (float)SPAN_USER_WEIGHT;
    usr->allocated_time_value = BASE_USER_TIME * usr->weight;
    usr->total_user_time = 0;
    printf("User ID: %d has allocated time %.5f \n", usr->uid, usr->allocated_time);

    usr->cnt_processes_available = 0;
    //usr->cnt_processes_incoming = rand() % SPAN_CNT_PROCESSES + 1;
    usr->cnt_processes_incoming =  3;

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
float update_available_processes(User* usr)
{
  Process* min_arrival_proc = usr->current_incoming;
  printf("User time %f\n",usr->total_user_time );
  for (int i = 0; i < usr->cnt_processes_incoming; i++)
  {
    if (usr->current_incoming->arrival_time < min_arrival_proc->arrival_time)
    {
      min_arrival_proc = usr->current_incoming;
    }
    printf("Process arival time %f\n",usr->current_incoming->arrival_time);
    if (usr->current_incoming->arrival_time <= usr->total_user_time)
    {
      usr->cnt_processes_incoming--;
      usr->cnt_processes_available++;
      usr->current_incoming->prev->next = usr->current_incoming->next;
      usr->current_incoming->next->prev = usr->current_incoming->prev;

      Process* aux_next = usr->current_incoming->next;

      if (usr->current_available == NULL)
      {
        usr->current_available = usr->current_incoming;
        usr->current_available->prev = NULL;
        usr->current_available->next = NULL;

      } else
      {
        Process* aux = usr->current_available->next;

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

float rr_processes_scheduler(User* usr)
{
  
  printf("RR:Processes for USER: %d with allocated time %f", usr->uid, usr->allocated_time);
  printf(" availiable %d and incomning %d\n",usr->cnt_processes_available,usr->cnt_processes_incoming);
  float retval = update_available_processes(usr);

  if (!retval)
  {
    Process* proc = usr->current_available;

    float available_user_time = usr->allocated_time;
    float time_quantum = TIME_QUANTUM;

    while (available_user_time > 0 && proc != NULL)
    {   
      // TODO
      float available_exec_time = min(available_user_time,time_quantum);
      float actual_exec_time = min(available_exec_time,proc->exec_time);
      
      printf("Pornim executia procesului cu exec time %f\n", proc->exec_time);
      
      kill(proc->pid, SIGCONT);
      sleep(actual_exec_time);
      kill(proc->pid, SIGSTOP);
      
      printf("exec_time %f actual_exec_time %f\n",proc->exec_time, actual_exec_time );
      proc->exec_time-=actual_exec_time;
      available_user_time-=actual_exec_time;
      
      printf("Oprim executia procesului cu exec time %f\n", proc->exec_time);
      
      if (proc->exec_time <= 0) {
        printf("Process %d has finished execution.\n", proc->pid);
        printf("Availaiable procces count %d\n",usr->cnt_processes_available);
        kill(proc->pid, SIGKILL);

        // Wait for the process to fully terminate
        waitpid(proc->pid, NULL, 0);

        if(usr->cnt_processes_available > 1){
          Process* aux = proc;
          proc->prev->next = proc->next;
          proc->next->prev = proc->prev;
          proc = proc->next;
          free(aux);
        }
        else{
          free(proc);
          proc = NULL;
        }
        usr->cnt_processes_available--;
      }
      else{
        proc = proc->next;
      }
    
    }

    return usr->allocated_time - available_user_time;
  } 
  else
  {
    printf("Am intrat pe else\n");
    float waited = min(retval, usr->allocated_time);
    printf("Retval %f, user %f, Waited %f\n",retval, usr->allocated_time,waited);
    //sleep(waited);
    usr->allocated_time -= waited;
    if (usr->allocated_time > 0)
    {
       return waited + rr_processes_scheduler(usr);
    } 
    return waited;
  }    
}

float wrr_users_scheduler(CPU* cpu)
{
  // checking if this works =(
  User* usr = cpu->current;
  
  while (usr != NULL)
  {
    usr->allocated_time = usr->allocated_time_value;
    usr->total_user_time += rr_processes_scheduler(usr);
    printf("WRR:Allocated time %f and total time %f for USER %d\n", usr->allocated_time, usr->total_user_time, usr->uid);
 
    if (usr->cnt_processes_incoming == 0 && usr->cnt_processes_available == 0)
    {
      printf("Am intrat aici inainte de segfault\n");
      User* aux = usr;
      usr->prev->next = usr->next;
      usr->next->prev = usr->prev;
      usr = usr->next;

      printf("User ID %d had a total time on CPU of %.5f \n", usr->uid, usr->total_user_time);
      //free(aux);

    } else usr = usr->next;
  }
  printf("Bye\n");
}

int main() 
{
  srand(time(NULL));

  for (int i = 0; i < CNT_CPUS; i++)
  {
    // generam nr de useri
    cpus[i].cpu_id = i;
    //cpus[i].cnt_users = rand() % SPAN_CNT_USERS + 1;
    cpus[i].cnt_users = 2;
    cpus[i].current = generate_users(&cpus[i]);
  }

  
  wrr_users_scheduler(&cpus[0]);
  
  return 0;
}
