size_t fwrite(const void *restrict, size_t, size_t, FILE *restrict);

// It looks like fwrite in bash is used first time for writing prompt. At this point terminal is set up.
size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream)
{
  size_t (*original_fwrite)() = (size_t (*)())dlsym(RTLD_NEXT, "fwrite");
  size_t result = original_fwrite(ptr, size, nitems, stream);
  if (is_bash && !is_terminal_setup) {
    add_PS1(socket_fd, getenv("NHI_PS1"));

    setenv("NHI_PROMPTER_PID", prompter_pid_str, 1);

    *environ_pointer = &environ;

    is_terminal_setup = true;
  }
  return result;
}
