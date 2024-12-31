#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#include "scheduler.h"

#define CNT_CPUS 1
#define BASE_USER_TIME 16
#define SPAN_PROCESS_ARRIVAL_TIME 50
#define SPAN_PROCESS_EXEC_TIME 30
#define SPAN_CNT_PROCESSES 8
#define SPAN_CNT_USERS 4
#define SPAN_USER_WEIGHT 100
#define TIME_QUANTUM 3

struct CPU cpus[CNT_CPUS];


static inline float min(float a, float b) {
    return (a < b) ? a : b;
}

void print_processes(struct User* usr){
    printf("Incoming : ");
    struct Process* ptr = usr->current_incoming;
    if(ptr != NULL){
      int start = ptr->pid;
      do{
        printf(" %d  ",ptr->pid);
        ptr=ptr->next;
        if(ptr==NULL){break;}
      }while(ptr->pid != start);
    }
    printf("\n");
    printf("Available: ");
    ptr = usr->current_available;
    if(ptr != NULL){
      int start = ptr->pid;
      do{
        printf(" %d  ",ptr->pid);
        ptr=ptr->next;
        if(ptr==NULL){break;}
      }while(ptr->pid != start);
    }
    printf("\n");
}

struct Process* generate_processes(struct User* usr)
{
  struct Process* prev = NULL;
  struct Process* head = NULL;

  for (int i = 0; i < usr->cnt_processes_incoming; i++)
  {
    struct Process* proc = malloc(sizeof(struct Process));
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
        printf("----Process %d running on CPU...\n", getpid());
        sleep(3);
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
    usr->allocated_time_value = BASE_USER_TIME * usr->weight;
    usr->total_user_time = 0;
    printf("User ID: %d has allocated time %.5f \n", usr->uid, usr->allocated_time_value);

    usr->cnt_processes_available = 0;
    usr->cnt_processes_incoming = rand() % SPAN_CNT_PROCESSES + 1;
    //usr->cnt_processes_incoming =  3;

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


float update_available_processes(struct User* usr)
{
  //printf("User time %f\n",usr->total_user_time ); 
  if(usr->current_incoming != NULL){

    struct Process* min_arrival_proc = usr->current_incoming;

    for (int i = 0; i < usr->cnt_processes_incoming; i++)
    {
      printf("Process %d arival time %f and exec time %f\n",usr->current_incoming->pid,usr->current_incoming->arrival_time,usr->current_incoming->exec_time);
    
      if (usr->current_incoming->arrival_time < min_arrival_proc->arrival_time)
      {
        min_arrival_proc = usr->current_incoming;
      }
      
      if (usr->current_incoming->arrival_time <= usr->total_user_time)
      {
        struct Process* available = usr->current_incoming;

        if(usr->cnt_processes_incoming > 1){
          usr->current_incoming->prev->next = usr->current_incoming->next;
          usr->current_incoming->next->prev = usr->current_incoming->prev;
          usr->current_incoming = usr->current_incoming->next;
        }
        else{
          usr->current_incoming = NULL;
        }

        i--;
        usr->cnt_processes_incoming--;
        usr->cnt_processes_available++;
        
        if (usr->current_available == NULL)
        {
          usr->current_available = available;
          usr->current_available->prev = available;
          usr->current_available->next = available;
        } 
        else
        {
          struct Process* aux = usr->current_available->next;

          if (aux == NULL)
          {
            // lista are doar 1 element
            usr->current_available->next = available;
            available->next = usr->current_available;
            available->prev = usr->current_available;
            usr->current_available->prev = available;
          } 
          else 
          {
            usr->current_available->next = available;
            available->prev = usr->current_available;
            available->next = aux;
            aux->prev = available;
          }
        }
        
      } 
      else
      { 
        usr->current_incoming = usr->current_incoming->next;
      }
    }
    
    if (usr->current_available == NULL)
    {
      return min_arrival_proc->arrival_time;
    } 
  }
  return 0;
}

float rr_processes_scheduler(struct User* usr)
{
  float retval = update_available_processes(usr);
  printf("RR: available %d and incoming %d\n",usr->cnt_processes_available, usr->cnt_processes_incoming);
  print_processes(usr);

  //sleep(2);

  if (retval == 0)
  {
    struct Process* proc = usr->current_available;
    printf("Procesul disponibil acum este %d cu %f exec time\n", proc->pid,proc->exec_time);

    float available_user_time = usr->allocated_time;
    float time_quantum = TIME_QUANTUM;

    while (available_user_time > 0 && proc != NULL)
    {   
      float available_exec_time = min(available_user_time,time_quantum);
      float actual_exec_time = min(available_exec_time,proc->exec_time);
      
      printf("Pornim executia procesului cu exec time %f\n", proc->exec_time);
    
      kill(proc->pid, SIGCONT);
      sleep(actual_exec_time);
      kill(proc->pid, SIGSTOP);
      
      proc->exec_time-=actual_exec_time;
      available_user_time-=actual_exec_time;
      
      printf("Oprim executia procesului cu exec time %f\n", proc->exec_time);
      //printf("S-a executat %f\n",actual_exec_time);
      
      if (proc->exec_time <= 0.001) {
        printf("\nSUCCESS: Process %d has finished execution.\n\n", proc->pid);
        kill(proc->pid, SIGKILL);

        // Wait for the process to fully terminate
        waitpid(proc->pid, NULL, 0);

        struct Process* aux = proc;          
        if(usr->cnt_processes_available > 1){
          proc->prev->next = proc->next;
          proc->next->prev = proc->prev;
          proc = proc->next;
        }
        else{
          proc = NULL;
        }
        free(aux);
        usr->cnt_processes_available--;
        //sleep(5);
      }
      else{
        proc = proc->next;
      }
    }
    usr->current_available = proc;
    return usr->allocated_time - available_user_time;
  } 
  else
  {
    printf("No process available now\n");
    float waited = min(retval, usr->allocated_time);
    printf("Retval %f, Waited %f\n",retval,waited);
    //sleep(waited);
    usr->allocated_time -= waited;
    if (usr->allocated_time > 0)
    {
       return waited + rr_processes_scheduler(usr);
    } 
    return waited;
  }   
}

float wrr_users_scheduler(struct CPU* cpu)
{
  struct User* usr = cpu->current;
  
  while (usr != NULL)
  {
    usr->allocated_time = usr->allocated_time_value;
    printf("\nWRR before: Allocated time %f and total time %f for USER %d\n", usr->allocated_time, usr->total_user_time, usr->uid);
    usr->total_user_time += rr_processes_scheduler(usr);
    printf("WRR  after: Allocated time %f and total time %f for USER %d\n", usr->allocated_time, usr->total_user_time, usr->uid);
     
    if (usr->cnt_processes_incoming == 0 && usr->cnt_processes_available == 0)
    {

      printf("\nSUCCESS: User ID %d finished and had a total time on CPU of %.5f \n\n", usr->uid, usr->total_user_time);  
      cpu->total_runtime += usr->total_user_time;

      struct User* aux = usr;
      if(cpu->cnt_users>1)
      {
        usr->prev->next = usr->next;
        usr->next->prev = usr->prev;
        usr = usr->next;
      }
      else{
        usr = NULL;
      }
      
      free(aux);
      cpu->cnt_users--;
    } 
    else {
      usr = usr->next;
    }
  }

  printf("Timpul pentrectut pe CPU %f\n",cpu->total_runtime);
  printf("Bye\n");
}

int main() 
{
  srand(time(NULL));

  for (int i = 0; i < CNT_CPUS; i++)
  {
    cpus[i].cpu_id = i;
    cpus[i].cnt_users = rand() % SPAN_CNT_USERS + 1;
    //cpus[i].cnt_users = 2;
    cpus[i].current = generate_users(&cpus[i]);
  }
  
  wrr_users_scheduler(&cpus[0]);
  
  return 0;
}
