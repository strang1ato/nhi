#define _GNU_SOURCE

#include "common.h"
#include "sqlite.h"
#include "utils.h"

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>

sqlite3 *db;

struct bpf_object *bpf_object;

int shells_fd;

char **__environ, **original_environ;

int handle_event(void *, void *, size_t);
void handle_kill_SIGUSR(struct kill_event *);
char ***get_shell_environ_address(pid_t, int);
void get_shell_environ(pid_t, char ***);
void reverse_environ(void);
void handle_kill_SIGRTMIN(struct kill_event *, size_t);
void handle_child_creation(pid_t *);
void handle_shell_exit(struct exit_shell_indicator_event *);
void handle_write(struct write_event *, size_t);

void destroy(void);
void sigint_hack(int);

int handle_event(void *ctx, void *data, size_t data_sz)
{
  if (data_sz == sizeof(struct kill_event)) {
    struct kill_event *kill_event = data;
    if (kill_event->sig == SIGUSR1 || kill_event->sig == SIGUSR2) {
      handle_kill_SIGUSR(kill_event);
    } else if (kill_event->sig == SIGRTMIN) {
      handle_kill_SIGRTMIN(kill_event, data_sz);
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

void handle_kill_SIGUSR(struct kill_event *kill_event)
{
  long indicator = get_indicator();
  if (kill_event->parent_shell_indicator) {
    char indicator_str[16];
    indicator_str[0] = -3;  // shell specificity
    sprintf(indicator_str+1, "%ld", indicator);
    add_output(db, kill_event->parent_shell_indicator, indicator_str, 12);  // indicator_str len is always 12
  }

  create_table(db, indicator);
  create_row(db, indicator);

  meta_create_row(db, indicator);

  int i = 0;
  for (int j = 0; j<SHELLS_MAX_ENTRIES; j++) {
    struct shell shell;
    bpf_map_lookup_elem(shells_fd, &i, &shell);
    if (!shell.shell_pid) {
      break;
    }
    i++;
  }

  struct shell helper;
  helper.shell_pid = kill_event->shell_pid;
  helper.indicator = indicator;
  helper.environ_address = get_shell_environ_address(kill_event->shell_pid, kill_event->sig);
  if (!helper.environ_address) {
    write_log("get_shell_environ_address failed at handle_kill_SIGUSR");
    return;
  }

  get_shell_environ(kill_event->shell_pid, helper.environ_address);

  add_PS1(db, indicator, getenv("NHI_PS1"));
  add_pwd(db, indicator, getenv("PWD"));

  if (sqlite3_wal_checkpoint(db, 0) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  reverse_environ();

  bpf_map_update_elem(shells_fd, &i, &helper, BPF_ANY);
}

char ***get_shell_environ_address(pid_t shell_pid, int signum)
{
  char environ_offset_str[32];
  {
    char command[128];
    if (signum == SIGUSR1) {
      strcpy(command, "objdump -T $(which bash) | awk -v sym=environ ' $NF == sym && $4 == \".bss\"  { print $1; exit }'");
    } else if (signum == SIGUSR2) {
      strcpy(command, "objdump -T $(which zsh) | awk -v sym=environ ' $NF == sym && $4 == \".bss\"  { print $1; exit }'");
    }
    FILE *process = popen(command, "r");
    if (!process) {
      write_log("popen at get_shell_environ_address failed");
      return 0;
    }

    if (!fgets(environ_offset_str, sizeof(environ_offset_str), process)) {
      write_log("fgets at get_shell_environ_address failed");
      return 0;
    }

    pclose(process);
  }
  long environ_offset = strtol(environ_offset_str, 0, 16);
  if (environ_offset == LONG_MIN) {
    write_log("strtol failed with underflow at get_shell_environ_address");
    return 0;
  } else if (environ_offset == LONG_MAX) {
    write_log("strtol failed with overflow at get_shell_environ_address");
    return 0;
  }

  char maps_content_str[64];
  {
    char maps[32];
    sprintf(maps, "/proc/%d/maps", shell_pid);
    FILE *file = fopen(maps, "r");
    if (!file) {
      write_log("fopen at get_shell_environ_address failed");
      return 0;
    }

    if (!fgets(maps_content_str, 64, file)) {
      write_log("fgets at get_shell_environ_address failed");
      return 0;
    }

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
  if (base_address == LONG_MIN) {
    write_log("strtol failed with underflow at get_shell_environ_address");
    return 0;
  } else if (base_address == LONG_MAX) {
    write_log("strtol failed with overflow at get_shell_environ_address");
    return 0;
  }
  return (char ***)(base_address + environ_offset);
}

void get_shell_environ(pid_t shell_pid, char ***shell_environ_address)
{
  char ***environ_pointer;
  {
    struct iovec local[1];
    local[0].iov_base = malloc(sizeof(char ***));
    local[0].iov_len = sizeof(char ***);

    struct iovec remote[1];
    remote[0].iov_base = shell_environ_address;
    remote[0].iov_len = sizeof(char ***);

    if (process_vm_readv(shell_pid, local, 1, remote, 1, 0) == -1) {
      write_log("process_vm_readv failed at get_shell_environ");
      return;
    }
    environ_pointer = local[0].iov_base;
  }

  char **__environ_addresses;
  {
    struct iovec local[1];
    local[0].iov_base = calloc(ENVIRON_AMOUNT_OF_VARIABLES, sizeof(char *));
    local[0].iov_len = ENVIRON_AMOUNT_OF_VARIABLES;

    struct iovec remote[1];
    remote[0].iov_base = *environ_pointer;
    remote[0].iov_len = ENVIRON_AMOUNT_OF_VARIABLES;

    __environ_addresses = local[0].iov_base;

    if (process_vm_readv(shell_pid, local, 1, remote, 1, 0) == -1) {
      write_log("process_vm_readv failed at get_shell_environ");
      return;
    }
  }

  free(environ_pointer);

  {
    __environ = calloc(LOCAL_ENVIRON_AMOUNT_OF_VARIABLES, sizeof(char *));

    struct iovec local[1];
    struct iovec remote[1];
    int j = 0;
    for (int i = 0; i<ENVIRON_AMOUNT_OF_VARIABLES; i++) {
      local[0].iov_base = calloc(1, ENVIRON_ELEMENT_SIZE);
      local[0].iov_len = ENVIRON_ELEMENT_SIZE;

      remote[0].iov_base = __environ_addresses[i];
      remote[0].iov_len = ENVIRON_ELEMENT_SIZE;

      if (process_vm_readv(shell_pid, local, 1, remote, 1, 0) == -1) {
        free(local[0].iov_base);
        break;
      }

      if (!strlen(local[0].iov_base)) {
        write_log("Shell environ is invalid");
        free(local[0].iov_base);
        return;
      }

      if (!strncmp(local[0].iov_base, "PWD", 3) || !strncmp(local[0].iov_base, "NHI_PS1", 7) || !strncmp(local[0].iov_base, "NHI_EXIT_STATUS", 15) || !strncmp(local[0].iov_base, "NHI_LAST_EXECUTED_COMMAND", 25)) {
        __environ[j] = local[0].iov_base;
        j++;
        if (j == LOCAL_ENVIRON_AMOUNT_OF_VARIABLES) {
          break;
        }
      } else {
        free(local[0].iov_base);
      }
    }
  }

  free(__environ_addresses);
}

void reverse_environ(void)
{
  for (int i = 0; i<LOCAL_ENVIRON_AMOUNT_OF_VARIABLES; i++) {
    free(__environ[i]);
  }
  free(__environ);

  __environ = original_environ;
}

void handle_kill_SIGRTMIN(struct kill_event *kill_event, size_t data_sz)
{
  // find indicator and environ_address of the shell
  long indicator = 0;
  char ***environ_address = 0;
  {
    int i = 0;
    for (int j = 0; j<SHELLS_MAX_ENTRIES; j++) {
      struct shell shell;
      bpf_map_lookup_elem(shells_fd, &i, &shell);
      if (shell.shell_pid == kill_event->shell_pid) {
        indicator = shell.indicator;
        environ_address = shell.environ_address;
        break;
      }
      i++;
    }
  }
  get_shell_environ(kill_event->shell_pid, environ_address);

  add_command(db, indicator, getenv("NHI_LAST_EXECUTED_COMMAND"));
  add_exit_status(db, indicator, getenv("NHI_EXIT_STATUS"));
  add_finish_time(db, indicator);
  add_indicator(db, indicator);

  create_row(db, indicator);

  add_PS1(db, indicator, getenv("NHI_PS1"));
  add_pwd(db, indicator, getenv("PWD"));

  if (sqlite3_wal_checkpoint(db, 0) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  reverse_environ();
}

void handle_child_creation(pid_t *shell_pid)
{
  // find indicator of the shell
  long indicator = 0;
  {
    int i = 0;
    for (int j = 0; j<SHELLS_MAX_ENTRIES; j++) {
      struct shell shell;
      bpf_map_lookup_elem(shells_fd, &i, &shell);
      if (shell.shell_pid == *shell_pid) {
        indicator = shell.indicator;
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

  if (sqlite3_wal_checkpoint(db, 0) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }
}

void handle_write(struct write_event *write_event, size_t data_sz)
{
  add_output(db, write_event->indicator, write_event->output, data_sz-sizeof(long));
}

__attribute__((destructor)) void destroy(void)
{
  bpf_object__close(bpf_object);
  sqlite3_close(db);
}

void sigint_hack(int signum)
{
  exit(0);
}

int main()
{
  signal(SIGINT, sigint_hack);

  original_environ = __environ;

  db = open_and_setup_db();
  if (!db) {
    return 0;
  }

  bpf_object = bpf_object__open_file("/etc/nhi/nhi.bpf.o", 0);
  if (libbpf_get_error(bpf_object)) {
    write_log("Failed to open bpf_object");
    return 0;
  }

  if (bpf_object__load(bpf_object)) {
    write_log("Failed to load bpf_object");
    return 0;
  }

  char *program_names[5] = {"kill_something_info", "fork_tracepoint", "do_exit", "ksys_write", "__fdget_pos"};
  for (int i=0; i<5; i++) {
    struct bpf_program *bpf_program;

    bpf_program = bpf_object__find_program_by_name(bpf_object, program_names[i]);
    if (!bpf_program) {
      write_log("Failed to find bpf_program");
      return 0;
    }

    struct bpf_link *bpf_link = bpf_program__attach(bpf_program);
    if (libbpf_get_error(bpf_link)) {
      write_log("Failed to attach bpf_program");
      return 0;
    }
    free(bpf_link);
  }

  shells_fd = bpf_object__find_map_fd_by_name(bpf_object, "shells");
  if (shells_fd == -EINVAL) {
    write_log("Failed to find shells map");
    return 0;
  }

  struct ring_buffer *ring_buffer = ring_buffer__new(bpf_object__find_map_fd_by_name(bpf_object, "ring_buffer"), handle_event, 0, 0);
  if (!ring_buffer) {
    write_log("Failed to create ring_buffer");
    return 0;
  }

  while (1) {
    ring_buffer__poll(ring_buffer, 1000);
  }
  return 0;
}
