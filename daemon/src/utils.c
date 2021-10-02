#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int write_log(const char *);
char *get_date(void);

int write_log(const char *message)
{
  int fd = open("/tmp/nhi.log", O_RDWR|O_APPEND|O_CREAT, "a");
  dprintf(fd, "%s | %s\n", get_date(), message);
  close(fd);
  return EXIT_SUCCESS;
}

char *get_date(void)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char *date = malloc(sizeof(char) * 20);
  sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return date;
}
