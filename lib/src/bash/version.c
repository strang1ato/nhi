int bash_history_fd;

bool completion, long_completion, after_question;

size_t fwrite(const void *restrict, size_t, size_t, FILE *restrict);

int open(const char *, int, mode_t);
ssize_t read(int, void *, size_t);
size_t strlen(const char *);

int __printf_chk(int, const char *restrict, ...);
int __fprintf_chk(FILE *, int, const char *, ...);

int putc(int, FILE *);
int puts(const char *);

ssize_t write(int, const void *, size_t);

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

    if (!strcmp(ptr, "^C") && size == 1 && nitems == 2 && stream == stderr) {
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
 * if buf is read from stdin and certain conditions are met change values of variables
 */
ssize_t read(int fd, void *buf, size_t count)
{
  ssize_t (*original_read)() = (ssize_t (*)())dlsym(RTLD_NEXT, "read");
  ssize_t result = original_read(fd, buf, count);

  if (is_bash && fd == 0 && (completion || long_completion)) {
    char lower_buf = tolower(((char *)buf)[0]);
    if (lower_buf != '\t') {
      if (after_question) {
        long_completion = false;
        after_question = false;
      }
      completion = false;
    }
    if (lower_buf == 'y') {
      after_question = true;
    }
    if (lower_buf == 'n') {
      long_completion = false;
    }
  }
  return result;
}

/*
 * if is_bash and "lead=${COMP_LINE:0:$COMP_POINT}" is provided as arg
 * we can be sure that completion is printing
 */
size_t strlen(const char *s)
{
  size_t (*original_strlen)() = (size_t (*)())dlsym(RTLD_NEXT, "strlen");
  size_t result = original_strlen(s);

  if (is_bash && !strcmp(s, "lead=${COMP_LINE:0:$COMP_POINT}")) {
    completion = true;
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

  if (is_bash && !completion && !long_completion && isatty(STDOUT_FILENO)) {
    va_start(args, format);
    char output[result];
    vsprintf(output, format, args);
    va_end(args);
    add_output(socket_fd, output, stdout_specificity);
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

  if (is_bash && ((stream == stdout && isatty(STDOUT_FILENO)) || (stream == stderr && isatty(STDERR_FILENO)))) {
    if (!strcmp(format, "Display all %d possibilities? (y or n)")) {
      long_completion = true;
      return result;
    }

    va_start(args, format);
    char output[result];
    vsprintf(output, format, args);
    va_end(args);
    if (stream == stdout) {
      add_output(socket_fd, output, stdout_specificity);
    } else {
      add_output(socket_fd, output, stderr_specificity);
    }
  }
  return result;
}

/*
 * putc function is used in bulitins, for example by printf.
 * putc is also used for deleting characters next to prompt
 */
int putc(int c, FILE *stream)
{
  int (*original_putc)() = (int (*)())dlsym(RTLD_NEXT, "putc");
  int result = original_putc(c, stream);

  if (is_bash && stream == stdout && isatty(STDOUT_FILENO) && !completion && !long_completion) {
    char *s = (char *)(&c);
    add_output(socket_fd, s, stdout_specificity);
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

  if (is_bash && isatty(STDOUT_FILENO)) {
    add_output(socket_fd, (char *) s, stdout_specificity);
  }
  return status;
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
    add_command(socket_fd, (char *) buffer, size);
    add_finish_time(socket_fd);
    add_indicator(socket_fd);

    create_row(socket_fd);

    bash_history_fd = 0;
  }
  return status;
}
