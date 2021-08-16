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

void add_command(int, char *, size_t);
void add_output(int, char *, char);
void add_pwd(int, char *);
void add_start_time(int);
void add_finish_time(int);
void add_indicator(int);

void meta_create_row(int, long, const char *);
void meta_add_finish_time(int, const char *);
char *get_date(void);

/*
 * setup_vars setups global reusable variables
 */
void setup_vars(const char *name)
{
  sprintf(create_row_query, "%s%s%s",
          "INSERT INTO `", name, "` VALUES (NULL, '', NULL, NULL, NULL, NULL);");
  create_row_query_len = strlen(create_row_query);
  sprintf(create_row_query_len_str, "%d", create_row_query_len);
  create_row_query_len_str_len = strlen(create_row_query_len_str);

  strcpy(table_name, name);
}

/*
 * connect_to_socket to sqlite-queue socket and returns socket fd
 */
int connect_to_socket(void)
{
  int socket_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  struct sockaddr_un socket_address;
  socket_address.sun_family = AF_UNIX;
  strcpy(socket_address.sun_path, "/tmp/sqlite-queue.socket");
  connect(socket_fd, (const struct sockaddr *) &socket_address, sizeof(socket_address));
  return socket_fd;
}

/*
 * close_socket closes connection to socket
 */
void close_socket(int socket_fd)
{
  write(socket_fd, "4e", 2);
  write(socket_fd, "exit", 4);
  close(socket_fd);
}

/*
 * create_table with given name
 */
void create_table(int socket_fd, const char *table_name)
{
  char query[110];
  sprintf(query, "%s%s%s",
          "CREATE TABLE `", table_name, "` (command TEXT, output BLOB, pwd TEXT, start_time TEXT, finish_time TEXT, indicator INTEGER);");
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

/*
 * create_row creates new empty row
 */
void create_row(int socket_fd)
{
  write(socket_fd, create_row_query_len_str, create_row_query_len_str_len);
  write(socket_fd, create_row_query, create_row_query_len);
}

/*
 * add_command adds executed command to the last row
 */
void add_command(int socket_fd, char *command, size_t size)
{
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

/*
 * add_output adds output to the last row
 */
void add_output(int socket_fd, char *data, char specificity)
{
  int size = strlen(data);
  for (int i = 0; i < size; i++) {
    if (data[i] == '\'') {
      data[i] = '\"';
    }
  }
  char query[110+size];
  if (specificity == -3) {
    sprintf(query, "%s%s%s%c%s%s%s%s%s",  /* there is no '' between data */
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

/*
 * add_pwd adds path to current working directory to the last row
 */
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

/*
 * add_start_time adds start time to the last row
 */
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

/*
 * add_finish_time adds finish time to the last row
 */
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

/*
 * add_indicator adds time in deciseconds since unix epoch to the last row
 */
void add_indicator(int socket_fd)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  time_t seconds_part = now.tv_sec*10;
  suseconds_t deciseconds_part = now.tv_usec/100000;
  time_t indicator = seconds_part + deciseconds_part;
  char query[150];
  sprintf(query, "%s%s%s%ld%s%s%s",
          "UPDATE `", table_name, "` SET indicator='", indicator, "' WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  int query_len = strlen(query);
  char query_len_str[10];
  sprintf(query_len_str, "%d", query_len);
  write(socket_fd, query_len_str, strlen(query_len_str));
  write(socket_fd, query, query_len);
}

/*
 * meta_create_row creates new row in meta table and fills it with indicator, name and start_time
 */
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

/*
 * add_finish_time adds finish time to the last row of meta table
 */
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

/*
 * get_date returns date with current time
 */
char *get_date(void)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char *date = malloc(20 * sizeof(char));
  sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return date;
}
