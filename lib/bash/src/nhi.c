#include "sqlite.h"

#include <ctype.h>
#include <dlfcn.h>
#include <semaphore.h>
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

int bash_history_fd;

bool completion, long_completion, after_question;

/*
 * set_is_bash checks if current process is bash and
 * if so sets is_bash global variable to true
 */
void set_is_bash()
{
  pid_t pid = getpid();
  char path[18];
  sprintf(path, "%s%d%s", "/proc/", pid, "/comm");
  FILE *stream = fopen(path, "r");

  char context[5];
  fgets(context, 5, stream);
  if (strcmp(context, "bash") == 0) {
    is_bash = true;
  }
  fclose(stream);
}

/*
 * init runs when shared library is loaded
 */
__attribute__((constructor)) void init()
{
  set_is_bash();
  /*
   * if process is bash open db, set table_name to time since epoch in seconds and
   * create new table with table_name with one row
   */
  if (is_bash) {
    db = open_db();
    sprintf(table_name, "%ld", time(NULL));
    setup_queries(table_name);
    create_table(db, table_name);
    create_row(db, table_name);
  }
}

/*
 * destroy runs when shared library is unloaded
 */
__attribute__((destructor)) void destroy()
{
  sqlite3_close(db);
}

/*
 * fwrite sets is_terminal_setup if is_bash and runs original shared library call.
 * It looks like fwrite in bash is used first time for writing prompt. At this point terminal is set up.
 */
size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream)
{
  size_t (*original_fwrite)() = (size_t (*)())dlsym(RTLD_NEXT, "fwrite");
  size_t result = original_fwrite(ptr, size, nitems, stream);
  if (is_bash) {
    is_terminal_setup = true;

    int fd = fileno(stream);
    if (!strcmp(ptr, "^C") && size == 1 && nitems == 2 && fd == 2) {
      long_completion = false;
    }
  }
  return result;
}

/*
 * open sets bash_history_fd if certain conditions are met and runs original shared library call.
 * bash_history_fd is set if open is used to open .bash_history file right after command execution.
 */
int open(const char *pathname, int flags, mode_t mode)
{
  int (*original_open)() = (int (*)())dlsym(RTLD_NEXT, "open");
  int result = original_open(pathname, flags, mode);
  if (is_bash && flags == 1025 && mode == 0600) {
    bash_history_fd = result;
  }
  return result;
}

/*
 * write adds proper fields and new row to db if certain conditions are met and runs original shared library call.
 * Proper fields to db are added if write is used to update .bash_history file right after command execution.
 */
ssize_t write(int filedes, const void *buffer, size_t size)
{
  ssize_t (*original_write)() = (ssize_t (*)())dlsym(RTLD_NEXT, "write");
  ssize_t status = original_write(filedes, buffer, size);
  if (is_bash && filedes == bash_history_fd) {
    add_command(db, table_name, buffer, size);
    add_finish_time(db, table_name);
    add_indicator(db, table_name);

    create_row(db, table_name);

    bash_history_fd = 0;
  }
  return status;
}

size_t strlen(const char *s)
{
  size_t (*original_strlen)() = (size_t (*)())dlsym(RTLD_NEXT, "strlen");
  size_t result = original_strlen(s);

  if (is_bash && !strcmp(s, "lead=${COMP_LINE:0:$COMP_POINT}")) {
    completion = true;
  }
  return result;
}

ssize_t read(int fd, void *buf, size_t count)
{
  ssize_t (*original_read)() = (ssize_t (*)())dlsym(RTLD_NEXT, "read");
  ssize_t result = original_read(fd, buf, count);

  if (is_bash && fd == 0) {
    char lower_buf = tolower(((char *)buf)[0]);
    if (lower_buf != '\t') {
      if (after_question) {
        long_completion = false;
        after_question = false;
      }
      completion = false;
    }
    if (long_completion && lower_buf == 'y') {
      after_question = true;
    }
    if (lower_buf == 'n') {
      long_completion = false;
    }
  }
  return result;
}

/*
 * __printf_chk function is used in bulitins, for example by echo or dirs
 * __printf_chk is used also for printing possible completions
 */
int __printf_chk(int flag, const char *restrict format, ...)
{
  va_list args;
  va_start(args, format);
  int result = __vprintf_chk(flag, format, args);
  va_end(args);

  if (is_bash && !completion && !long_completion) {
    va_start(args, format);
    char output[result];
    vsprintf(output, format, args);
    va_end(args);
    add_output(db, table_name, output);
  }
  return result;
}

/*
 * __fprintf_chk function is used in bulitins, for example by times.
 * __fprintf_chk is also used for displaying long completion question
 */
int __fprintf_chk(FILE *stream, int flag, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int result = __vfprintf_chk(stream, flag, format, args);
  va_end(args);

  int fd = fileno(stream);
  if (is_bash && (fd == STDOUT_FILENO || fd == STDERR_FILENO)) {
    if (!strcmp(format, "Display all %d possibilities? (y or n)")) {
      long_completion = true;
      return result;
    }

    va_start(args, format);
    char output[result];
    vsprintf(output, format, args);
    va_end(args);
    add_output(db, table_name, output);
  }
  return result;
}

/*
 * putc function is used in bulitins, for example by printf.
 * putc is also used for deleteing characters next to prompt
 */
int putc(int c, FILE *stream)
{
  int (*original_putc)() = (int (*)())dlsym(RTLD_NEXT, "putc");
  int result = original_putc(c, stream);

  int fd = fileno(stream);
  if (is_bash && fd == STDOUT_FILENO && !completion && !long_completion) {
    char *s = (char *)(&c);
    add_output(db, table_name, s);
  }
  return result;
}

/*
 * puts function is used in bulitins, for example by pwd
 */
int puts(const char *s)
{
  int (*original_puts)() = (int (*)())dlsym(RTLD_NEXT, "puts");
  int status = original_puts(s);

  if (is_bash) {
    add_output(db, table_name, s);
  }
  return status;
}

/*
 * execve attaches tracer and runs original shared library call.
 * execve is used by bash to execute program(s) defined in command.
 */
int execve(const char *pathname, char *const argv[], char *const envp[])
{
  sem_t *sem;

  pid_t tracer_pid = -1;  /* Set tracer_pid to any value but not zero */
  if (is_terminal_setup) {
    sem = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    sem_init(sem, 1, 0);

    tracer_pid = fork();
    if (!tracer_pid) {
      db = open_db();
      add_start_time(db, table_name);

      pid_t tracee_pid = getppid();
      int wstatus;
      int one_time = false;
      ptrace(PTRACE_ATTACH, tracee_pid, NULL, NULL);

      sem_post(sem);

      while(1) {
        waitpid(tracee_pid, &wstatus, 0);

        /*
         * Quick solution for receiving the same syscall 2 times in a row
         * When I have more free time I will try to figure it out and fix it in more elegant way.
         */
        if (!one_time) {
          one_time = true;
        } else {
          one_time = false;
          ptrace(PTRACE_SYSCALL, tracee_pid, NULL, NULL);
          continue;
        }

        if (wstatus == -1) {
          exit(EXIT_FAILURE);
        }
        if (WIFEXITED(wstatus)) {
          exit(EXIT_SUCCESS);
        }

        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, tracee_pid, NULL, &regs);
        if(regs.orig_rax == SYS_write && (regs.rdi == 1 || regs.rdi == 2)) {
          struct iovec local[1];
          local[0].iov_base = calloc(regs.rdx, sizeof(char));
          local[0].iov_len = regs.rdx;

          struct iovec remote[1];
          remote[0].iov_base = (void *)regs.rsi;
          remote[0].iov_len = regs.rdx;

          process_vm_readv(tracee_pid, local, 1, remote, 1, 0);
          add_output(db, table_name, local[0].iov_base);
        }

        ptrace(PTRACE_SYSCALL, tracee_pid, NULL, NULL);
      }
    }
  }

  if (tracer_pid) {
    if (is_terminal_setup) {
      sem_wait(sem);
      sem_destroy(sem);
      munmap(sem, sizeof(sem_t));
    }

    int (*original_execve)() = (int (*)())dlsym(RTLD_NEXT, "execve");
    int status = original_execve(pathname, argv, envp);
    return status;
  }
  return 0;
}
