#define SHELL_PIDS_AND_INDICATORS_MAX_ENTRIES 256
#define CHILD_PIDS_AND_SHELL_PIDS_MAX_ENTRIES 1024
#define RING_BUFFER_MAX_ENTRIES 1024*1024

#define KSYS_WRITE_EVENT_SIZE 32*1024

#define NHID_HASH 'n'*'h'*'i'*'d'  // simple ASCII hash that equals to 120120000

#define SIGUSR1 10
#define SIGUSR2 12

#define ENVIRON_AMOUNT_OF_VARIABLES 512
#define ENVIRON_ELEMENT_SIZE 512

typedef int pid_t;

struct kill_event {  // size 16
  pid_t shell_pid;
  int sig;
  long parent_shell_indicator;
};

struct exit_shell_indicator_event {  // size 9
  long indicator;
  char _;  // add char just to make size unique
};

struct write_event {
  long indicator;
  char output[];
};

struct shell_pid_and_indicator {
  pid_t shell_pid;
  long indicator;
  char ***environ_address;
};

struct child_pid_and_shell_pid {
  pid_t child_pid;
  pid_t shell_pid;
};
