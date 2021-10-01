#define _GNU_SOURCE

#include "common.h"
#include "sqlite.h"

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>

sqlite3 *db;
struct bpf_object *bpf_object;

int shell_pids_fd;
int child_pids_fd;

char **environ, **__environ;

int handle_event(void *, void *, size_t);
void handle_kill_SIGUSR1(struct kill_event *);
long create_indicator(void);
char ***get_shell_environ_address(pid_t);
char **get_shell_environ(pid_t, char ***);
char *get_shell_environ_var(char **, char *);
void handle_kill_SIGUSR2(struct kill_event *, size_t);
void handle_child_creation(pid_t *);
void handle_shell_exit(struct exit_shell_indicator_event *);
void handle_write(struct write_event *, size_t);

int handle_event(void *ctx, void *data, size_t data_sz)
{
  if (data_sz == sizeof(struct kill_event)) {
    struct kill_event *kill_event = data;
    if (kill_event->sig == SIGUSR1) {
      handle_kill_SIGUSR1(kill_event);
    } else if (kill_event->sig == SIGUSR2) {
      handle_kill_SIGUSR2(kill_event, data_sz);
    }
  } else if (data_sz == sizeof(pid_t)) {
    handle_child_creation(data);
  } else if (data_sz == sizeof(struct exit_shell_indicator_event)) {
    handle_shell_exit(data);
  } else {
    handle_write(data, data_sz);
  }
  return 0;
}

void handle_kill_SIGUSR1(struct kill_event *kill_event)
{
  long indicator = create_indicator();
  create_table(db, indicator);
  create_row(db, indicator);

  meta_create_row(db, indicator);

  int i = 0;
  for (int j = 0; j<SHELL_PIDS_AND_INDICATORS_MAX_ENTRIES; j++) {
    struct shell_pid_and_indicator shell_pid_and_indicator;
    bpf_map_lookup_elem(shell_pids_fd, &i, &shell_pid_and_indicator);
    if (!shell_pid_and_indicator.shell_pid) {
      break;
    }
    i++;
  }

  struct shell_pid_and_indicator helper;
  helper.shell_pid = kill_event->shell_pid;
  helper.indicator = indicator;
  helper.environ_address = get_shell_environ_address(kill_event->shell_pid);

  environ = get_shell_environ(kill_event->shell_pid, helper.environ_address);
  __environ = environ;

  add_PS1(db, indicator, getenv("NHI_PS1"));

  bpf_map_update_elem(shell_pids_fd, &i, &helper, BPF_ANY);
}

long create_indicator(void)
{
  struct timeval now;
  gettimeofday(&now, 0);
  time_t seconds_part = now.tv_sec*10;
  suseconds_t deciseconds_part = now.tv_usec/100000;
  long indicator = seconds_part + deciseconds_part;  // indicator is time in deciseconds since unix epoch
  return indicator;
}

char ***get_shell_environ_address(pid_t shell_pid)
{
  char environ_offset_str[32];
  {
    FILE *process = popen("objdump -tT $(which bash) | awk -v sym=environ ' $NF == sym && $4 == \".bss\"  { print $1; exit}'", "r");
    fgets(environ_offset_str, sizeof(environ_offset_str), process);
    pclose(process);
  }
  long environ_offset = strtol(environ_offset_str, 0, 16);

  char maps_content_str[64];
  {
    char maps[32];
    sprintf(maps, "/proc/%d/maps", shell_pid);
    FILE *file = fopen(maps, "r");

    fgets(maps_content_str, 64, file);
    fclose(file);
  }

  char base_address_str[64];
  for (int i=0; i<64; i++) {
    if (maps_content_str[i] == '-') {
      base_address_str[i] = 0;
      break;
    }
    base_address_str[i] = maps_content_str[i];
  }
  long base_address = strtol(base_address_str, 0, 16);
  return (char ***)(base_address + environ_offset);
}

char **get_shell_environ(pid_t shell_pid, char ***shell_environ_address)
{
  char ***environ_pointer;
  {
    struct iovec local[1];
    local[0].iov_base = calloc(sizeof(char ***), 1);
    local[0].iov_len = sizeof(char ***);

    struct iovec remote[1];
    remote[0].iov_base = shell_environ_address;
    remote[0].iov_len = sizeof(char ***);

    process_vm_readv(shell_pid, local, 1, remote, 1, 0);
    environ_pointer = local[0].iov_base;
  }

  char **environ;
  {
    struct iovec local[1];
    local[0].iov_base = calloc(ENVIRON_AMOUNT_OF_VARIABLES, sizeof(char *));
    local[0].iov_len = ENVIRON_AMOUNT_OF_VARIABLES;

    struct iovec remote[1];
    remote[0].iov_base = *environ_pointer;
    remote[0].iov_len = ENVIRON_AMOUNT_OF_VARIABLES;

    environ = local[0].iov_base;

    process_vm_readv(shell_pid, local, 1, remote, 1, 0);
  }

  {
    struct iovec local[ENVIRON_AMOUNT_OF_VARIABLES];
    struct iovec remote[ENVIRON_AMOUNT_OF_VARIABLES];
    for (int i = 0; i<ENVIRON_AMOUNT_OF_VARIABLES; i++) {
      local[i].iov_base = calloc(ENVIRON_ELEMENT_SIZE, 1);
      local[i].iov_len = ENVIRON_ELEMENT_SIZE;

      remote[i].iov_base = environ[i];
      remote[i].iov_len = ENVIRON_ELEMENT_SIZE;

      environ[i] = local[i].iov_base;
    }

    process_vm_readv(shell_pid, local, ENVIRON_AMOUNT_OF_VARIABLES, remote, ENVIRON_AMOUNT_OF_VARIABLES, 0);
  }
  return environ;
}

void handle_kill_SIGUSR2(struct kill_event *kill_event, size_t data_sz)
{
  // find indicator and environ_address of the shell
  long indicator = 0;
  char ***environ_address = 0;
  {
    int i = 0;
    for (int j = 0; j<SHELL_PIDS_AND_INDICATORS_MAX_ENTRIES; j++) {
      struct shell_pid_and_indicator shell_pid_and_indicator;
      bpf_map_lookup_elem(shell_pids_fd, &i, &shell_pid_and_indicator);
      if (shell_pid_and_indicator.shell_pid == kill_event->shell_pid) {
        indicator = shell_pid_and_indicator.indicator;
        environ_address = shell_pid_and_indicator.environ_address;
        break;
      }
      i++;
    }
  }
  environ = get_shell_environ(kill_event->shell_pid, environ_address);
  __environ = environ;

  add_command(db, indicator, getenv("NHI_LAST_EXECUTED_COMMAND"));
  add_pwd(db, indicator, getenv("PWD"));
  add_finish_time(db, indicator);
  add_indicator(db, indicator);

  create_row(db, indicator);

  add_PS1(db, indicator, getenv("NHI_PS1"));
}

void handle_child_creation(pid_t *shell_pid)
{
  // find indicator of the shell
  long indicator = 0;
  {
    int i = 0;
    for (int j = 0; j<SHELL_PIDS_AND_INDICATORS_MAX_ENTRIES; j++) {
      struct shell_pid_and_indicator shell_pid_and_indicator;
      bpf_map_lookup_elem(shell_pids_fd, &i, &shell_pid_and_indicator);
      if (shell_pid_and_indicator.shell_pid == *shell_pid) {
        indicator = shell_pid_and_indicator.indicator;
        break;
      }
      i++;
    }
  }

  add_start_time(db, indicator);
}

void handle_shell_exit(struct exit_shell_indicator_event *exit_shell_indicator_event)
{
  meta_add_finish_time(db, exit_shell_indicator_event->indicator);
}

void handle_write(struct write_event *write_event, size_t data_sz)
{
  add_output(db, write_event->indicator, write_event->output);
}

__attribute__((destructor)) void destroy(void)
{
  bpf_object__close(bpf_object);
  sqlite3_close(db);
}

int main()
{
  db = open_db();

  bpf_object = bpf_object__open_file("nhi.bpf.o", 0);
  if (libbpf_get_error(bpf_object)) {
    return 0;
  }

  if (bpf_object__load(bpf_object)) {
    return 0;
  }

  char *program_names[5] = {"kill_something_info", "kernel_clone_ret", "do_exit", "ksys_write", "__fdget_pos"};
  for (int i=0; i<5; i++) {
    struct bpf_program *bpf_program;

    bpf_program = bpf_object__find_program_by_name(bpf_object, program_names[i]);
    if (!bpf_program) {
      return 0;
    }

    if (libbpf_get_error(bpf_program__attach(bpf_program))) {
      return 0;
    }
  }

  shell_pids_fd = bpf_object__find_map_fd_by_name(bpf_object, "shell_pids_and_indicators");
  child_pids_fd = bpf_object__find_map_fd_by_name(bpf_object, "child_pids_and_shell_pids");

  struct ring_buffer *ring_buffer = ring_buffer__new(bpf_object__find_map_fd_by_name(bpf_object, "ring_buffer"), handle_event, 0, 0);
  if (!ring_buffer) {
    return 0;
  }

  while (1) {
    ring_buffer__poll(ring_buffer, 1000);
  }
  return 0;
}
