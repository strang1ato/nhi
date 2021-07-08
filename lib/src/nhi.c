#include "sqlite.h"

#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

sqlite3 *db;
char table_name[11];

bool is_bash, is_terminal_setup;

char stdout_specificity,
     stderr_specificity,
     shell_specificity;

int bash_history_fd;

bool completion, long_completion, after_question;

bool (*is_fd_tty)[1024];

char tty_name[15];

char *get_proc_name(pid_t pid)
{
  char path[18];
  sprintf(path, "%s%d%s", "/proc/", pid, "/comm");
  if (access(path, F_OK)) {
    return NULL;
  }
  FILE *stream = fopen(path, "r");
  char context[11];
  fgets(context, 11, stream);
  fclose(stream);
  char *name = context;
  return name;
}

/*
 * init runs when shared library is loaded
 */
__attribute__((constructor)) void init(void)
{
  stdout_specificity = -1;
  stderr_specificity = -2;
  shell_specificity = -3;
  {
    char *proc_name = get_proc_name(getpid());
    if (!strcmp(proc_name, "bash\n")) {
      is_bash = true;
    }
  }

  /*
   * if process is bash set new table_name based on current time
   */
  if (is_bash) {
    db = open_db();

    long current_time = time(NULL);
    sprintf(table_name, "%ld", current_time);
    setup_queries(table_name);

    create_table(db, table_name);
    create_row(db);

    meta_create_row(db, current_time, table_name);

    pid_t tracer_pid = getpid() + 1;
    char *proc_name = get_proc_name(tracer_pid);
    if (proc_name && !strcmp(proc_name, "nhi-tracer")) {
      kill(tracer_pid, SIGUSR1);
    }

    setenv("NHI_CURRENT_SHELL_INDICATOR", table_name, 1);
  } else {
    sprintf(table_name, "%s", getenv("NHI_CURRENT_SHELL_INDICATOR"));
    setup_queries(table_name);
  }

  if (ttyname(STDIN_FILENO)) {
    sprintf(tty_name, "%s", ttyname(STDIN_FILENO));
  }
}

/*
 * destroy runs when shared library is unloaded
 */
__attribute__((destructor)) void destroy(void)
{
  if (is_bash) {
    meta_add_finish_time(db, table_name);
  }

  sqlite3_close(db);
}

#include "bash/version.c"

/*
 * fetch_string fetches and returns string from given tracee address
 */
char *fetch_string(pid_t pid, size_t local_iov_len, void *remote_iov_base)
{
  struct iovec local[1];
  local[0].iov_base = calloc(local_iov_len, sizeof(char));
  local[0].iov_len = local_iov_len;

  struct iovec remote[1];
  remote[0].iov_base = remote_iov_base;
  remote[0].iov_len = local_iov_len;

  process_vm_readv(pid, local, 1, remote, 1, 0);
  return local[0].iov_base;
}

/*
 * fork creates new process and creates and attaches tracer to newly created process
 */
pid_t fork(void)
{
  pid_t (*original_fork)() = (pid_t (*)())dlsym(RTLD_NEXT, "fork");

  if (!is_terminal_setup && is_bash) {
    pid_t result = original_fork();
    return result;
  }

  is_fd_tty = mmap(NULL, sizeof(bool)*1024, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

  pid_t tracee_pid = original_fork();

  if (!tracee_pid) {
    sem_t *sem = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    sem_init(sem, 1, 0);

    pid_t tracer_pid = original_fork();
    if (!tracer_pid) {
      void set_shell_reference(int signum) {
        add_output(db, get_latest_indicator(db), &shell_specificity);
        exit(EXIT_SUCCESS);
      }
      signal(SIGUSR1, set_shell_reference);

      is_bash = false;
      pid_t pid = getpid();
      char path[18];
      sprintf(path, "%s%d%s", "/proc/", pid, "/comm");
      FILE *stream = fopen(path, "w");
      fputs("nhi-tracer", stream);
      fclose(stream);

      /*
       * Close all inherited file descriptors (except 0, 1, 2).
       * 1024 is default maximum number of fds that can be opened by process in linux.
       */
      for (int i = 3; i < 1024; i++) {
        close(i);
      }

      db = open_db();

      add_start_time(db);

      pid_t tracee_pid = getppid();

      int wstatus;

      bool second_time = false;

      ptrace(PTRACE_ATTACH, tracee_pid, NULL, NULL);

      sem_post(sem);

      while(1) {
        waitpid(tracee_pid, &wstatus, 0);

        if (errno == ECHILD) {
          munmap(is_fd_tty, sizeof(bool)*1024);
          break;
        }

        int signal;
        if (WIFSTOPPED(wstatus)) {
          signal = WSTOPSIG(wstatus);
          if (signal == SIGTRAP || signal == SIGSTOP) {
            signal = 0;
          }
        }

        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, tracee_pid, NULL, &regs);
        if (second_time) {
          if (regs.rax != -1) {
            switch (regs.orig_rax) {
            case SYS_write:
              if (regs.rdi < 0 || regs.rdi >= 1024) {
                break;
              }
              if ((*is_fd_tty)[regs.rdi]) {
                char *data = fetch_string(tracee_pid, regs.rdx, (void *)regs.rsi);
                if (regs.rdi == 2) {
                  add_output(db, data, &stderr_specificity);
                } else {
                  add_output(db, data, &stdout_specificity);
                }
                free(data);
              }
              break;
            case SYS_dup2:
            case SYS_dup3:
              if ((regs.rdi < 0 || regs.rdi >= 1024) || (regs.rsi < 0 || regs.rsi >= 1024)) {
                break;
              }
              if ((*is_fd_tty)[regs.rdi]) {
                (*is_fd_tty)[regs.rsi] = true;
              }
              break;
            case SYS_dup:
              if ((regs.rdi < 0 || regs.rdi >= 1024) || (regs.rax < 0 || regs.rax >= 1024)) {
                break;
              }
              if ((*is_fd_tty)[regs.rdi]) {
                (*is_fd_tty)[regs.rax] = true;
              }
              break;
            case SYS_fcntl:
              if ((regs.rdi < 0 || regs.rdi >= 1024) || (regs.rax < 0 || regs.rax >= 1024)) {
                break;
              }
              if (!regs.rsi && (*is_fd_tty)[regs.rdi]) {
                (*is_fd_tty)[regs.rax] = true;
              }
              break;
            case SYS_open: {
              if (regs.rax < 0 || regs.rax >= 1024) {
                break;
              }
              char *data = fetch_string(tracee_pid, 1024, (void *)regs.rdi);
              if (!strcmp(data, tty_name)) {
                (*is_fd_tty)[regs.rax] = true;
              }
              free(data);
              break;
            }
            case SYS_openat: {
              if (regs.rax < 0 || regs.rax >= 1024) {
                break;
              }
              char *data = fetch_string(tracee_pid, 1024, (void *)regs.rsi);
              if (!strcmp(data, tty_name)) {
                (*is_fd_tty)[regs.rax] = true;
              }
              free(data);
              break;
            }
            case SYS_close:
              if (regs.rdi < 0 || regs.rdi >= 1024) {
                break;
              }
              (*is_fd_tty)[regs.rdi] = false;
              break;
            }
          }

          second_time = false;
        } else {
          second_time = true;
        }

        ptrace(PTRACE_SYSCALL, tracee_pid, NULL, signal);
      }

      exit(EXIT_SUCCESS);
    } else {
      sem_wait(sem);
      sem_destroy(sem);
      munmap(sem, sizeof(sem_t));
    }
  }
  return tracee_pid;
}

/*
 * execve sets "inter-process" variables indicating if fd refers to tty and executes original shared library call.
 * execve is used by bash to execute program(s) defined in command.
 */
int execve(const char *pathname, char *const argv[], char *const envp[])
{
  if (isatty(STDOUT_FILENO)) {
    (*is_fd_tty)[STDOUT_FILENO] = true;
  }
  if (isatty(STDERR_FILENO)) {
    (*is_fd_tty)[STDERR_FILENO] = true;
  }

  int (*original_execve)() = (int (*)())dlsym(RTLD_NEXT, "execve");
  int status = original_execve(pathname, argv, envp);
  return status;
}
