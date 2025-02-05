#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

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

pthread_mutex_t user_mutex;
pthread_cond_t process_cond;
sem_t cpu_semaphore;

int logging;
volatile sig_atomic_t shutdown_requested = 0;

static inline float min(float a, float b)
{
    return (a < b) ? a : b;
}

void init_sync()
{
  pthread_mutex_init(&user_mutex, NULL);
  pthread_cond_init(&process_cond, NULL);
  sem_init(&cpu_semaphore, 0, 1); // single CPU
}

void cleanup_sync()
{
  pthread_mutex_destroy(&user_mutex);
  pthread_cond_destroy(&process_cond);
  sem_destroy(&cpu_semaphore);
}

void print_in_file(const char *format, ...)
{
    char buffer[4096];  
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args); 
    va_end(args);

    write(logging, buffer, strlen(buffer));
}

void print_users(struct CPU* cpu) 
{
  print_in_file("\n");
  struct User* ptr = cpu->current;
    if(ptr != NULL)
    {
      int start = ptr->uid;
      do
      {
        print_in_file("User ID %d has allocated time %.2f and %d processes\n", ptr->uid, ptr->allocated_time_value, ptr->cnt_processes_incoming);
        ptr = ptr->next;
        if(ptr == NULL)
        {
          break;
        }
      } while(ptr->uid != start);
    }
    print_in_file("\n");
}

void print_processes(struct User* usr)
{
    printf("Available: ");
    struct Process* ptr = usr->current_available;
    if(ptr != NULL)
    {
      int start = ptr->pid;
      do
      {
        printf(" %d  ",ptr->pid);
        ptr = ptr->next;
        if(ptr == NULL)
        {
          break;
        }
      }while(ptr->pid != start);
    }

    printf("\n");
    printf("Incoming : ");

    ptr = usr->current_incoming;
    if(ptr != NULL)
    {
      int start = ptr->pid;
      do
      {
        printf(" %d  ",ptr->pid);
        ptr = ptr->next;
        if(ptr == NULL)
        {
          break;
        }
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
    if (proc == NULL)
    {
      perror("Malloc failed for process");
      exit(1);
    }

    proc->uid = usr->uid;
    proc->exec_time = rand() % SPAN_PROCESS_EXEC_TIME + 1;
    proc->arrival_time = rand() % SPAN_PROCESS_ARRIVAL_TIME + 1;
    //printf("Exec %d, Arrival %d\n",proc->exec_time, proc->arrival_time);
    pid_t pid = fork();

    if (pid == 0)
    {
      print_in_file("Process %d ( user %d ): arrival time %.2f and execution time %.2f\n", getpid(), usr->uid, proc->arrival_time, proc->exec_time);
      kill(getpid(), SIGSTOP);
      while(true)
      {
        printf("-----> Process %d running on CPU...\n", getpid());
        sleep(3);
      }
  
    } else if (pid > 0)
    {
      proc->pid = pid;
      
    } else {
      perror("fork failed");
      free(proc);
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
    if (usr == NULL)
    {
      perror("Malloc failed for user");
      exit(1);
    }

    usr->uid = i;
    // user->username = "user" + i;
    usr->weight = ((rand() % SPAN_USER_WEIGHT) + 10 ) / (float)SPAN_USER_WEIGHT;
    usr->allocated_time_value = BASE_USER_TIME * usr->weight;
    usr->total_user_time = 0;
    
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

float update_available_processes(struct User* usr)
{
  pthread_mutex_lock(&user_mutex);

  if(usr->current_incoming != NULL){

    struct Process* min_arrival_proc = usr->current_incoming;

    for (int i = 0; i < usr->cnt_processes_incoming; i++)
    {
      printf("Process %d arival time %.2f and exec time %.2f\n",usr->current_incoming->pid,usr->current_incoming->arrival_time,usr->current_incoming->exec_time);
    
      if (usr->current_incoming->arrival_time < min_arrival_proc->arrival_time)
      {
        min_arrival_proc = usr->current_incoming;
      }
      
      if (usr->current_incoming->arrival_time <= usr->total_user_time)
      {
        struct Process* available = usr->current_incoming;

        if(usr->cnt_processes_incoming > 1)
        {
          usr->current_incoming->prev->next = usr->current_incoming->next;
          usr->current_incoming->next->prev = usr->current_incoming->prev;
          usr->current_incoming = usr->current_incoming->next;
        }
        else
        {
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
      pthread_mutex_unlock(&user_mutex);
      return min_arrival_proc->arrival_time;
    } 
  }

  pthread_mutex_unlock(&user_mutex);
  return 0;
}

float rr_processes_scheduler(struct User* usr)
{
  pthread_mutex_lock(&user_mutex);

  float retval = update_available_processes(usr);
  printf("RR Processes: %d available and %d incoming\n",usr->cnt_processes_available, usr->cnt_processes_incoming);
  print_processes(usr);

  if (retval == 0)
  {
    struct Process* proc = usr->current_available;
    //printf("Procesul %d disponibil -> %.2f exec time\n", proc->pid,proc->exec_time);

    float available_user_time = usr->allocated_time;
    float time_quantum = TIME_QUANTUM;

    while (available_user_time > 0 && proc != NULL)
    {   
      float available_exec_time = min(available_user_time,time_quantum);
      float actual_exec_time = min(available_exec_time,proc->exec_time);
      sem_wait(&cpu_semaphore);
      
      printf("Start execution -> exec time %.2f\n", proc->exec_time);
    
      kill(proc->pid, SIGCONT);
      sleep(actual_exec_time);
      kill(proc->pid, SIGSTOP);
      
      proc->exec_time-=actual_exec_time;
      available_user_time-=actual_exec_time;
      
      printf("Stop execution -> exec time %.2f\n", proc->exec_time);

      sem_post(&cpu_semaphore);
      
      if (proc->exec_time <= 0.001)
      {
        printf("\nSUCCESS: Process %d (%d) has finished execution.\n\n", proc->pid, proc->uid);
        print_in_file("SUCCESS: Process %d (%d) has finished execution.\n", proc->pid, proc->uid);

        kill(proc->pid, SIGKILL);
        // Wait for the process to fully terminate
        waitpid(proc->pid, NULL, 0);

        struct Process* aux = proc;          
        if(usr->cnt_processes_available > 1)
        {
          proc->prev->next = proc->next;
          proc->next->prev = proc->prev;
          proc = proc->next;
        }
        else
        {
          proc = NULL;
        }
        free(aux);
        usr->cnt_processes_available--;
        //sleep(5);
      }
      else
      {
        proc = proc->next;
      }
    }
    usr->current_available = proc;

    pthread_mutex_unlock(&user_mutex);
    return usr->allocated_time - available_user_time;
  } 
  else
  {
    printf("No process available now\n");
    float waited = min(retval, usr->allocated_time);
    printf("Retval %.2f, Waited %.2f\n",retval,waited);
    //sleep(waited);
    usr->allocated_time -= waited;
    if (usr->allocated_time > 0)
    {
       return waited + rr_processes_scheduler(usr);
    } 

    pthread_mutex_unlock(&user_mutex);
    return waited;
  }   

  pthread_mutex_unlock(&user_mutex);
}

float wrr_users_scheduler(struct CPU* cpu)
{
  pthread_mutex_lock(&user_mutex);
  struct User* usr = cpu->current;
  
  while (usr != NULL && !shutdown_requested)
  {
    usr->allocated_time = usr->allocated_time_value;
    printf("\n----------------------------------------------\n");
    printf("WRR Users: Allocated time %.2f and total time %.2f for USER %d\n", usr->allocated_time, usr->total_user_time, usr->uid);
    usr->total_user_time += rr_processes_scheduler(usr);
    printf("WRR Users: Allocated time %.2f and total time %.2f for USER %d\n", usr->allocated_time, usr->total_user_time, usr->uid);
    printf("----------------------------------------------\n\n");

    if (usr->cnt_processes_incoming == 0 && usr->cnt_processes_available == 0)
    {
      cpu->total_runtime += usr->total_user_time;

      printf("\nSUCCESS: User ID %d finished and had a total time on CPU of %.2f \n\n", usr->uid, usr->total_user_time);  
      print_in_file("\nSUCCESS: User ID %d finished and had a total time on CPU of %.2f \n", usr->uid, usr->total_user_time);  
      print_in_file("CPU time is %.2f\n\n",cpu->total_runtime);

      struct User* aux = usr;
      if(cpu->cnt_users>1)
      {
        usr->prev->next = usr->next;
        usr->next->prev = usr->prev;
        usr = usr->next;
      }
      else
      {
        usr = NULL;
      }
      
      free(aux);
      cpu->cnt_users--;
    } 
    else
    {
      usr = usr->next;
    }
  }

  if (shutdown_requested)
  {
    printf("Scheduler interrupted for shutdown.\n");
  }

  print_in_file("Total CPU time %.2f\n",cpu->total_runtime);
  printf("Total CPU time %.2f\n",cpu->total_runtime);

  pthread_mutex_unlock(&user_mutex);
}

void handle_sigchld(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

void handle_sigint(int sig)
{
  printf("Shutdown initiated...\n");
  shutdown_requested = 1;

  for (int i = 0; i < CNT_CPUS; i++)
  {
    struct User* usr = cpus[i].current;
    if (usr != NULL)
    {
      do
      { 
        struct Process* proc = usr->current_available;
        while (proc != NULL)
        {
          printf("Terminating process %d...\n", proc->pid);
          kill(proc->pid, SIGKILL);
          waitpid(proc->pid, NULL, 0);

          struct Process* next = proc->next;
          free(proc);
          if (next == proc) break;
          proc = next;
        }

        struct User* next_user = usr->next;
        free(usr);
        if (next_user == usr) break;
        usr = next_user;
      } while(usr != cpus[i].current);
    }
  }

  close(logging);
  cleanup_sync();

  printf("Shutdown complete.\n");
  exit(0);
}

void memory_cleanup()
{
  for (int i = 0; i < CNT_CPUS; i++)
  {
    struct User* usr = cpus[i].current;
    while (usr != NULL)
    {
      struct Process* proc = usr->current_available;
      while (proc != NULL)
      {
        struct Process* next_proc = proc->next;
        free(proc);
        proc = next_proc;
      }
      struct User* next_user = usr->next;
      free(usr);
      usr = next_user;
    }
  }
}

int main() 
{
  signal(SIGCHLD, handle_sigchld);
  signal(SIGINT, handle_sigint);

  srand(time(NULL));
  
  init_sync();  

  logging = open("./logging.txt",O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if(logging < 0)
  {
    char msg[50] = "Can't open destination file\n";
    write(2, msg, sizeof(msg));
    return 1;
  }

  printf("Starting..\n");
  for (int i = 0; i < CNT_CPUS; i++)
  {
    cpus[i].cpu_id = i;
    cpus[i].cnt_users = rand() % SPAN_CNT_USERS + 1;
    cpus[i].current = generate_users(&cpus[i]);
  }
  
  sleep(5);
  print_users(&cpus[0]);

  wrr_users_scheduler(&cpus[0]);

  memory_cleanup();
  close(logging);
  cleanup_sync();

  return 0;
}
