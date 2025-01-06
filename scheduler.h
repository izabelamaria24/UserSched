#ifndef SCHEDULER_H
#define SCHEDULER_H

typedef struct CPU
{
  int cpu_id; // ID-ul CPU-ului
  float total_runtime; // timpul total petrecut pe CPU
  
  int cnt_users; // numarul total de useri
    
  struct User *current; // userul de pe CPU
  
  struct User* (*generate_users)(struct CPU*); // generate users
  float (*wrr_users_scheduler)(struct CPU*); // schedule users
  void (*print_users)(struct CPU*) // in file
};

typedef struct User
{
  int uid; // user ID
  char username[16]; // username
  float weight; // user weight
  float allocated_time_value;
  float allocated_time;
  float total_user_time;

  int cnt_processes_incoming, cnt_processes_available; // numarul total de procese
  

  struct Process* current_incoming, *current_available; 
  struct User* prev; // pointer catre userul anterior
  struct User* next; // pointer catre userul urmator              

  struct Process* (*generate_processes)(struct User*); // generate processes
  float (*rr_processes_scheduler)(struct User*); // schedule processes
  void (*update_available_processes)(struct User*); 
  void (*print_processes)(struct User*); // in console
};

typedef struct Process
{
  pid_t pid; // ID-ul procesului
  int uid; // ID-ul userului corespunzator
  float exec_time; // timpul necesar pentru a se executa
  float arrival_time; // timpul la care ajunge pe CPU
  
  struct Process *prev; // pointer catre procesul anterior
  struct Process *next; // pointer catre procesul urmator
};


#endif // SCHEDULER_H
