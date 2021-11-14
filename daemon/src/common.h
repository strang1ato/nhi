#define SHELLS_MAX_ENTRIES 256
#define CHILDREN_MAX_ENTRIES 1024
#define RING_BUFFER_MAX_ENTRIES 1024*1024

#define KSYS_WRITE_EVENT_SIZE 32*1024

#define NHID_HASH 'n'*'h'*'i'*'d'  // simple ASCII hash that equals to 120120000

#define SIGUSR1 10
#define SIGUSR2 12
#define SIGRTMIN 34

#define ENVIRON_AMOUNT_OF_VARIABLES 8*1024
#define ENVIRON_ELEMENT_SIZE 512
#define LOCAL_ENVIRON_AMOUNT_OF_VARIABLES 4

typedef int pid_t;

struct kill_event {  // size 16
  pid_t shell_pid;
  int sig;
  long parent_shell_indicator;
};

struct exit_shell_indicator_event {  // size 8
  long indicator;
};

struct write_event {  // realistically at least size 11
  long indicator;
  char output[];
};

struct shell {
  pid_t shell_pid;
  long indicator;
  char omit_write;
  char ***environ_address;
};

struct child {
  pid_t child_pid;
  pid_t shell_pid;
};
