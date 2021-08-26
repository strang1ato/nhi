#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

char table_name[10];

char create_row_query[100];
int create_row_query_len;
char create_row_query_len_str[10];
int create_row_query_len_str_len;

void setup_vars(const char *);

int connect_to_socket(void);
void close_socket(int);

void create_table(int, const char *);

void create_row(int);

void add_PS1(int, char *);
void add_command(int, char *);
void add_output(int, char *, char, size_t);
void add_pwd(int, char *);
void add_start_time(int);
void add_finish_time(int);
void add_indicator(int);

void meta_create_row(int, long, const char *);
void meta_add_finish_time(int, const char *);
char *get_date(void);


// setup_vars setups global reusable variables
void setup_vars(const char *name)
{
  sprintf(create_row_query, "%s%s%s",
          "INSERT INTO `", name, "` VALUES (NULL, NULL, '', NULL, NULL, NULL, NULL);");
  create_row_query_len = strlen(create_row_query);
  sprintf(create_row_query_len_str, "%d", create_row_query_len);
  create_row_query_len_str_len = strlen(create_row_query_len_str);

  strcpy(table_name, name);
}

int connect_to_socket(void)
{
  int socket_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  struct sockaddr_un socket_address;
  socket_address.sun_family = AF_UNIX;
  strcpy(socket_address.sun_path, "/tmp/sqlite-queue.socket");
  if (connect(socket_fd, (const struct sockaddr *) &socket_address, sizeof(socket_address)) == -1) {
    dprintf(2, "Can not connect to sqlite-queue socket\n");
  }
  return socket_fd;
}

void close_socket(int socket_fd)
{
  write(socket_fd, "4e", 2);
  write(socket_fd, "exit", 4);
  close(socket_fd);
}

void create_table(int socket_fd, const char *table_name)
{
  char query[130];
  sprintf(query, "%s%s%s",
          "CREATE TABLE `", table_name, "` (PS1 TEXT, command TEXT, output BLOB, pwd TEXT, start_time TEXT, finish_time TEXT, indicator INTEGER);");
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

void create_row(int socket_fd)
{
  write(socket_fd, create_row_query_len_str, create_row_query_len_str_len);
  write(socket_fd, create_row_query, create_row_query_len);
}

void add_PS1(int socket_fd, char *PS1)
{
  char query[100+strlen(PS1)];
  sprintf(query, "%s%s%s%s%s%s%s",
          "UPDATE `", table_name, "` SET PS1='", PS1, "'WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

void add_command(int socket_fd, char *command)
{
  size_t size = strlen(command);
  char query[100+size];
  for (int i = 0; i < size; i++) {
    if (command[i] == '\'') {
      command[i] = '\"';
    }
  }
  sprintf(query, "%s%s%s%s%s%s%s",
          "UPDATE `", table_name, "` SET command='", command, "' WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

void add_output(int socket_fd, char *data, char specificity, size_t size)
{
  for (int i = 0; i < size; i++) {
    if (data[i] == '\'') {
      data[i] = '\"';
    }
  }
  char query[110+size];
  if (specificity == -3) {
    sprintf(query, "%s%s%s%c%s%s%s%s%s",  // there is no '' between data
            "UPDATE `", table_name, "` SET output=output || '", specificity, "' || ", data, " WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  } else {
    sprintf(query, "%s%s%s%c%s%s%s%s%s",
            "UPDATE `", table_name, "` SET output=output || '", specificity, "' || '", data, "' WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  }
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

void add_pwd(int socket_fd, char *pwd)
{
  char query[100+strlen(pwd)];
  sprintf(query, "%s%s%s%s%s%s%s",
          "UPDATE `", table_name, "` SET pwd='", pwd, "'WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

void add_start_time(int socket_fd)
{
  char query[150];
  char *date = get_date();
  sprintf(query, "%s%s%s%s%s%s%s",
          "UPDATE `", table_name, "` SET start_time='", date, "' WHERE start_time is NULL AND rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  free(date);
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

void add_finish_time(int socket_fd)
{
  char query[150];
  char *date = get_date();
  sprintf(query, "%s%s%s%s%s%s%s",
          "UPDATE `", table_name, "` SET finish_time='", date, "'WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  free(date);
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

void add_indicator(int socket_fd)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  time_t seconds_part = now.tv_sec*10;
  suseconds_t deciseconds_part = now.tv_usec/100000;
  time_t indicator = seconds_part + deciseconds_part;  // indicator is time in deciseconds since unix epoch
  char query[150];
  sprintf(query, "%s%s%s%ld%s%s%s",
          "UPDATE `", table_name, "` SET indicator='", indicator, "' WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

void meta_create_row(int socket_fd, long indicator, const char *name)
{
  char query[150];
  char *date = get_date();
  sprintf(query, "%s%ld%s%s%s%s%s",
          "INSERT INTO `meta` VALUES ('", indicator,"', '", name, "', '", date, "', NULL);");
  free(date);
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

void meta_add_finish_time(int socket_fd, const char *name)
{
  char query[150];
  char *date = get_date();
  sprintf(query, "%s%s%s%s%s",
          "UPDATE `meta` SET finish_time='", date, "' WHERE name='", name, "';");
  free(date);
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

char *get_date(void)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char *date = malloc(20);
  sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return date;
}
