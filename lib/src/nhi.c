#include "sqlite_queue_client.h"

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

int socket_fd;
char table_name[11];

bool is_bash, is_terminal_setup;
pid_t bash_pid;

char stdout_specificity,
     stderr_specificity,
     shell_specificity;

bool (*is_fd_tty)[1024];

char tty_name[15];

char ****environ_pointer;

void init(void);
char *get_proc_name(pid_t);
void *get_data_from_other_process(pid_t, size_t, void *);

void destroy(void);

pid_t fork(void);

int execve(const char *, char *const [], char *const []);

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
    free(proc_name);
  }

  if (is_bash) {
    bash_pid = getpid();
    socket_fd = connect_to_socket();

    long current_time = time(NULL);
    sprintf(table_name, "%ld", current_time);
    setup_vars(table_name);

    create_table(socket_fd, table_name);
    create_row(socket_fd);

    meta_create_row(socket_fd, current_time, table_name);

    pid_t tracer_pid = getpid() + 1;
    char *proc_name = get_proc_name(tracer_pid);
    if (proc_name && !strcmp(proc_name, "nhi-tracer")) {
      kill(tracer_pid, SIGUSR1);
    }
    free(proc_name);

    environ_pointer = mmap(NULL, sizeof(char ***), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pid_t prompter_pid = fork();
    if (!prompter_pid) {
      socket_fd = connect_to_socket();
      void prompter(int signum) {
        char ***original_environ = get_data_from_other_process(getppid(), sizeof(char ***), *environ_pointer);

        {
          struct iovec local[1];
          local[0].iov_base = calloc(512, sizeof(char *));
          local[0].iov_len = 512;

          struct iovec remote[1];
          remote[0].iov_base = *original_environ;
          remote[0].iov_len = 512;

          environ = local[0].iov_base;

          process_vm_readv(getppid(), local, 1, remote, 1, 0);
        }

        {
          struct iovec local[512];
          struct iovec remote[512];
          for (int i = 0; i<512; i++) {
            local[i].iov_base = calloc(512, 1);
            local[i].iov_len = 512;

            remote[i].iov_base = environ[i];
            remote[i].iov_len = 512;

            environ[i] = local[i].iov_base;
          }

          process_vm_readv(getppid(), local, 512, remote, 512, 0);
        }

        char *last_executed_command = getenv("NHI_LAST_EXECUTED_COMMAND");
        if (!last_executed_command) {
          last_executed_command = "history does not work";
        }

        int i = 1;
        while(1) {
          if (last_executed_command[i] >= 48 && last_executed_command[i] <= 57) {
            i++;
          } else {
            i+=2;
            break;
          }
        }

        add_command(socket_fd, last_executed_command+i, strlen(last_executed_command+i));
        add_pwd(socket_fd, getenv("PWD"));
        add_finish_time(socket_fd);
        add_indicator(socket_fd);

        create_row(socket_fd);
        add_PS1(socket_fd, getenv("NHI_PS1"));

        for (int i = 0; i<512; i++) {
          free(environ[i]);
        }
        free(environ);
        free(original_environ);
      }
      signal(SIGUSR1, prompter);
      while(1) {
        if (getppid() == 1) {
          close_socket(socket_fd);
          exit(EXIT_SUCCESS);
        }
        sleep(1);
      }
    }

    char prompter_pid_str[7];
    sprintf(prompter_pid_str, "%d", prompter_pid);
    setenv("NHI_PROMPTER_PID", prompter_pid_str, 1);
    setenv("NHI_CURRENT_SHELL_INDICATOR", table_name, 1);
  } else {
    sprintf(table_name, "%s", getenv("NHI_CURRENT_SHELL_INDICATOR"));
    setup_vars(table_name);
  }

  if (ttyname(STDIN_FILENO)) {
    sprintf(tty_name, "%s", ttyname(STDIN_FILENO));
  }
}

char *get_proc_name(pid_t pid)
{
  char path[18];
  sprintf(path, "%s%d%s", "/proc/", pid, "/comm");
  if (access(path, F_OK)) {
    return NULL;
  }
  FILE *stream = fopen(path, "r");
  char *name = malloc(11);
  fgets(name, 11, stream);
  fclose(stream);
  return name;
}

void *get_data_from_other_process(pid_t pid, size_t local_iov_len, void *remote_iov_base)
{
  struct iovec local[1];
  local[0].iov_base = calloc(local_iov_len, 1);
  local[0].iov_len = local_iov_len;

  struct iovec remote[1];
  remote[0].iov_base = remote_iov_base;
  remote[0].iov_len = local_iov_len;

  process_vm_readv(pid, local, 1, remote, 1, 0);
  return local[0].iov_base;
}

__attribute__((destructor)) void destroy(void)
{
  if (is_bash && bash_pid == getpid()) {  // double check is required, because child process may fail to overwrite is_bash
    meta_add_finish_time(socket_fd, table_name);
    close_socket(socket_fd);
  }
}

#include "bash/version.c"

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
    is_bash = false;

    sem_t *sem = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    sem_init(sem, 1, 0);

    pid_t tracer_pid = original_fork();
    if (!tracer_pid) {
      // Close all inherited file descriptors (except 0, 1, 2).
      // 1024 is default maximum number of fds that can be opened by process in linux.
      for (int i = 3; i < 1024; i++) {
        close(i);
      }

      int socket_fd = connect_to_socket();

      add_start_time(socket_fd);

      void set_shell_reference(int signum) {
        char *param = "(SELECT name FROM `meta` ORDER BY rowid DESC LIMIT 1)";
        add_output(socket_fd, param, shell_specificity, 60);
        close_socket(socket_fd);
        exit(EXIT_SUCCESS);
      }
      signal(SIGUSR1, set_shell_reference);

      pid_t pid = getpid();
      char path[18];
      sprintf(path, "%s%d%s", "/proc/", pid, "/comm");
      FILE *stream = fopen(path, "w");
      fputs("nhi-tracer", stream);
      fclose(stream);

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
                char *data = get_data_from_other_process(tracee_pid, regs.rdx, (void *)regs.rsi);
                if (regs.rdi == 2) {
                  add_output(socket_fd, data, stderr_specificity, regs.rdx);
                } else {
                  add_output(socket_fd, data, stdout_specificity, regs.rdx);
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
              char *data = get_data_from_other_process(tracee_pid, 1024, (void *)regs.rdi);
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
              char *data = get_data_from_other_process(tracee_pid, 1024, (void *)regs.rsi);
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

      close_socket(socket_fd);

      exit(EXIT_SUCCESS);
    } else {
      sem_wait(sem);
      sem_destroy(sem);
      munmap(sem, sizeof(sem_t));
    }
  }
  return tracee_pid;
}

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
