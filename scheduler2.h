#ifndef SCHEDULER_H
#define SCHEDULER_H

typedef struct Process
{
  pid_t pid; // ID-ul procesului
  int uid; // ID-ul userului corespunzator
  int exec_time; // timpul necesar pentru a se executa
  int arrival_time; // timpul la care ajunge pe CPU
  
  Process *prev; // pointer catre procesul anterior
  Process *next; // pointer catre procesul urmator
}Process;

typedef struct User
{
  int uid; // user ID
  char username[16]; // username
  float weight; // user weight
  float allocated_time;
  float allocated_time_value; 
  float total_user_time;

  int cnt_processes_incoming, cnt_processes_available; // numarul total de procese
  

  Process *current_incoming;
  Process *current_available; 
  User* prev; // pointer catre userul anterior
  User* next; // pointer catre userul urmator              

  Process* (*generate_processes)(User*); // generate processes
  float (*rr_processes_scheduler)(User*); // schedule processes
  void (*update_available_processes)(User*);
}User;

typedef struct CPU
{
  int cpu_id; // ID-ul CPU-ului
  float total_runtime; // timpul total petrecut pe CPU
  
  int cnt_users; // numarul total de useri
    
  User *current; // userul de pe CPU
  
  User* (*generate_users)(CPU*); // generate users
  float (*wrr_users_scheduler)(CPU*); // schedule users
}CPU;



#endif // SCHEDULER_H
