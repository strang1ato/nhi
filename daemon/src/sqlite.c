#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

sqlite3 *open_db(void);
int write_error_to_log_file(sqlite3 *, char *, char *);

void create_table(sqlite3 *, long);

void create_row(sqlite3 *, long);

void add_PS1(sqlite3 *, long, const void *);
void add_command(sqlite3 *, long, const void *);
void add_output(sqlite3 *, long, const void *);
void add_pwd(sqlite3 *, long, const void *);
void add_start_time(sqlite3 *, long);
char *get_date(void);
void add_finish_time(sqlite3 *, long);
void add_indicator(sqlite3 *, long);

void meta_create_row(sqlite3 *, long);
void meta_add_finish_time(sqlite3 *, long);

sqlite3 *open_db(void)
{
  sqlite3 *db;
  char *db_path = "/home/karol/.nhi/db";
  if (sqlite3_open(db_path, &db) != SQLITE_OK) {
    write_error_to_log_file(db, "open_db", "sqlite3_open");
  }
  return db;
}

int write_error_to_log_file(sqlite3 *db, char *external_function, char *internal_function)
{
  char *log_path = "/tmp/nhi.log";
  if (!log_path) {
    return EXIT_FAILURE;
  }
  FILE *log_fd = fopen(log_path, "a");
  char message[200];
  sprintf(message,
          "%s function executed within %s returned error message: %s\n", internal_function, external_function, sqlite3_errmsg(db));
  fputs(message, log_fd);
  fclose(log_fd);
  return EXIT_SUCCESS;
}

void create_table(sqlite3 *db, long indicator)
{
  sqlite3_stmt *stmt;
  char query[150];
  sprintf(query, "%s%ld%s",
          "CREATE TABLE `", indicator, "` (PS1 TEXT, command TEXT, output BLOB, pwd TEXT, start_time TEXT, finish_time TEXT, indicator INTEGER);");
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "create_table", "sqlite3_prepare_v2");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "create_table", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

void create_row(sqlite3 *db, long indicator)
{
  char query[100];
  sprintf(query, "%s%ld%s",
          "INSERT INTO `", indicator, "` VALUES (NULL, NULL, '', NULL, NULL, NULL, NULL);");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "create_row", "sqlite3_prepare_v2");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "create_row", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

void add_PS1(sqlite3 *db, long indicator, const void *PS1)
{
  char query[100];
  sprintf(query, "%s%ld%s%ld%s",
          "UPDATE `", indicator, "` SET PS1=? WHERE rowid = (SELECT MAX(rowid) FROM `", indicator, "`);");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_PS1", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_text(stmt, 1, PS1, strlen(PS1), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_PS1", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_PS1", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

void add_command(sqlite3 *db, long indicator, const void *command)
{
  char query[100];
  sprintf(query, "%s%ld%s%ld%s",
          "UPDATE `", indicator, "` SET command=? WHERE rowid = (SELECT MAX(rowid) FROM `", indicator, "`);");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_command", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_text(stmt, 1, command, strlen(command), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_command", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_command", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

void add_output(sqlite3 *db, long indicator, const void *data)
{
  char query[100];
  sprintf(query, "%s%ld%s%ld%s",
          "UPDATE `", indicator, "` SET output=output || ? WHERE rowid = (SELECT MAX(rowid) FROM `", indicator, "`);");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_output", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_blob(stmt, 1, data, strlen(data), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_output", "sqlite3_bind_blob");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_output", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

void add_pwd(sqlite3 *db, long indicator, const void *pwd)
{
  char query[100];
  sprintf(query, "%s%ld%s%ld%s",
          "UPDATE `", indicator, "` SET pwd=? WHERE rowid = (SELECT MAX(rowid) FROM `", indicator, "`);");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_pwd", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_text(stmt, 1, pwd, strlen(pwd), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_pwd", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_pwd", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

void add_start_time(sqlite3 *db, long indicator)
{
  char query[100];
  sprintf(query, "%s%ld%s%ld%s",
          "UPDATE `", indicator, "` SET start_time=? WHERE start_time is NULL AND rowid = (SELECT MAX(rowid) FROM `", indicator, "`);");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_start_time", "sqlite3_prepare_v2");
  }

  char *date = get_date();
  if (sqlite3_bind_text(stmt, 1, date, strlen(date), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_start_time", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_start_time", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

char *get_date(void)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char *date = malloc(sizeof(char) * 20);
  sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return date;
}

void add_finish_time(sqlite3 *db, long indicator)
{
  char query[100];
  sprintf(query, "%s%ld%s%ld%s",
          "UPDATE `", indicator, "` SET finish_time=? WHERE rowid = (SELECT MAX(rowid) FROM `", indicator, "`);");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_finish_time", "sqlite3_prepare_v2");
  }

  char *date = get_date();
  if (sqlite3_bind_text(stmt, 1, date, strlen(date), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_finish_time", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_finish_time", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

void add_indicator(sqlite3 *db, long indicator)
{
  char query[100];
  sprintf(query, "%s%ld%s%ld%s",
          "UPDATE `", indicator, "` SET indicator=? WHERE rowid = (SELECT MAX(rowid) FROM `", indicator, "`);");

  struct timeval now;
  gettimeofday(&now, NULL);
  time_t seconds_part = now.tv_sec*10;
  suseconds_t deciseconds_part = now.tv_usec/100000;
  time_t command_indicator = seconds_part + deciseconds_part;
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_indicator", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_int64(stmt, 1, command_indicator) != SQLITE_OK) {
    write_error_to_log_file(db, "add_indicator", "sqlite3_bind_int64");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_indicator", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

void meta_create_row(sqlite3 *db, long indicator)
{
  sqlite3_stmt *stmt;
  char *query = "INSERT INTO `meta` VALUES (?1, ?2, ?3, NULL);";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_create_row", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_int64(stmt, 1, indicator) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_create_row", "sqlite3_bind_int64");
  }
  char name[16];
  sprintf(name, "%ld", indicator);
  if (sqlite3_bind_text(stmt, 2, name, strlen(name), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_create_row", "sqlite3_bind_text");
  }
  char *date = get_date();
  if (sqlite3_bind_text(stmt, 3, date, strlen(date), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_create_row", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "meta_create_row", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

void meta_add_finish_time(sqlite3 *db, long indicator)
{
  sqlite3_stmt *stmt;
  char *query = "UPDATE `meta` SET finish_time=?1 WHERE indicator=?2;";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_add_finish_time", "sqlite3_prepare_v2");
  }

  char *date = get_date();
  if (sqlite3_bind_text(stmt, 1, date, strlen(date), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_add_finish_time", "sqlite3_bind_text");
  }
  if (sqlite3_bind_int64(stmt, 2, indicator) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_add_finish_time", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "meta_add_finish_time", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}
