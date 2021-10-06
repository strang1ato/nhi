#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int write_log(const char *);
long get_indicator(void);

int write_log(const char *message)
{
  int fd = open("/tmp/nhi.log", O_RDWR|O_APPEND|O_CREAT, "a");

  time_t t = time(0);
  struct tm tm = *localtime(&t);
  char *date = malloc(20);
  sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

  dprintf(fd, "%s | %s\n", date, message);

  free(date);

  close(fd);
  return 0;
}

long get_indicator(void)
{
  struct timeval now;
  gettimeofday(&now, 0);
  time_t seconds_part = now.tv_sec*10;
  suseconds_t deciseconds_part = now.tv_usec/100000;
  long indicator = seconds_part + deciseconds_part;  // indicator is time in deciseconds since unix epoch
  return indicator;
}
