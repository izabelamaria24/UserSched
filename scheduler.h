#ifndef SCHEDULER_H
#define SCHEDULER_H

typedef struct CPU
{
  int cpu_id; // ID-ul CPU-ului
  float total_run_time; // timpul total petrecut pe CPU
  
  int cnt_users; // numarul total de useri
    
  struct User *current; // userul de pe CPU
  
  struct User* (*generate_users)(struct CPU *, int cnt_users); // generate users
};

typedef struct User
{
  int uid; // user ID
  char username[16]; // username
  float weight; // user weight
  int cnt_processes; // numarul total de procese

  struct Process *current; // procesul curent

  struct User *prev; // pointer catre userul anterior
  struct User *next; // pointer catre userul urmator
                     
  struct Process* (*generate_processes)(struct User*, int cnt_processes); // generate processes
};

typedef struct Process
{
  pid_t pid; // ID-ul procesului
  int uid; // ID-ul userului corespunzator
  int exec_time; // timpul necesar pentru a se executa
  int arrival_time; // timpul la care ajunge pe CPU
  
  struct Process *prev; // pointer catre procesul anterior
  struct Process *next; // pointer catre procesul urmator
};


#endif // SCHEDULER_H
