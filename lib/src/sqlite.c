#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

char create_row_query[100],
     add_command_query[100],
     add_output_query[100],
     add_start_time_query[100],
     add_finish_time_query[100],
     add_indicator_query[100];

sqlite3 *open_db();

void setup_queries(const char *);

void create_table(sqlite3 *, const char *);

void create_row(sqlite3 *);

void add_command(sqlite3 *, const char *, size_t);
void add_output(sqlite3 *, const char *, const char *);
void add_start_time(sqlite3 *);
void add_finish_time(sqlite3 *);
void add_indicator(sqlite3 *);

void meta_create_row(sqlite3 *, long, const char *);
void meta_add_finish_time(sqlite3 *, const char *);
char *get_date();
int write_error_to_log_file(sqlite3 *, char *, char *);

char *get_latest_indicator(sqlite3 *);

/*
 * setup_queries setups global variables containing reusable queries
 */
void setup_queries(const char *table_name)
{
  sprintf(create_row_query, "%s%s%s",
          "INSERT INTO `", table_name, "` VALUES (NULL, '', NULL, NULL, NULL);");
  sprintf(add_command_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET command=? WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  sprintf(add_output_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET output=output || ?1 || ?2 WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  sprintf(add_start_time_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET start_time=? WHERE start_time is NULL AND rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  sprintf(add_finish_time_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET finish_time=? WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  sprintf(add_indicator_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET indicator=? WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
}

/*
 * open_db opens nhi database
 */
sqlite3 *open_db()
{
  sqlite3 *db;
  char *db_path = getenv("NHI_DB_PATH");
  if (sqlite3_open(db_path, &db) != SQLITE_OK) {
    write_error_to_log_file(db, "open_db", "sqlite3_open");
  }
  return db;
}

/*
 * create_table with given name
 */
void create_table(sqlite3 *db, const char *table_name)
{
  sqlite3_stmt *stmt;
  char query[110];
  sprintf(query, "%s%s%s",
          "CREATE TABLE `", table_name, "` (command TEXT, output BLOB, start_time TEXT, finish_time TEXT, indicator INTEGER);");
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "create_table", "sqlite3_prepare_v2");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "create_table", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

/*
 * create_row creates new empty row
 */
void create_row(sqlite3 *db)
{
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, create_row_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "create_row", "sqlite3_prepare_v2");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "create_row", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

/*
 * add_command adds executed command to the last row
 */
void add_command(sqlite3 *db, const char *command, size_t size)
{
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_command_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_command", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_text(stmt, 1, command, size, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_command", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_command", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

/*
 * add_output adds output to the last row
 */
void add_output(sqlite3 *db, const char *data, const char *specificity)
{
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_output_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_output", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_blob(stmt, 1, specificity, 1, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_output", "sqlite3_bind_text");
  }
  if (sqlite3_bind_blob(stmt, 2, data, strlen(data), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_output", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_output", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

/*
 * add_start_time adds start time to the last row
 */
void add_start_time(sqlite3 *db)
{
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_start_time_query, -1, &stmt, NULL) != SQLITE_OK) {
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

/*
 * add_finish_time adds finish time to the last row
 */
void add_finish_time(sqlite3 *db)
{
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_finish_time_query, -1, &stmt, NULL) != SQLITE_OK) {
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

/*
 * add_indicator adds time in deciseconds since unix epoch to the last row
 */
void add_indicator(sqlite3 *db)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  time_t seconds_part = now.tv_sec*10;
  suseconds_t deciseconds_part = now.tv_usec/100000;
  time_t indicator = seconds_part + deciseconds_part;
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_indicator_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "add_indicator", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_int64(stmt, 1, indicator) != SQLITE_OK) {
    write_error_to_log_file(db, "add_indicator", "sqlite3_bind_int64");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "add_indicator", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

/*
 * meta_create_row creates new row in meta table and fills it with indicator, name and start_time
 */
void meta_create_row(sqlite3 *db, long indicator, const char *name)
{
  sqlite3_stmt *stmt;
  char *query = "INSERT INTO `meta` VALUES (?1, ?2, ?3, NULL);";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_create_row", "sqlite3_prepare_v2");
  }

  if (sqlite3_bind_int64(stmt, 1, indicator) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_create_row", "sqlite3_bind_int64");
  }
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

/*
 * add_finish_time adds finish time to the last row of meta table
 */
void meta_add_finish_time(sqlite3 *db, const char *name)
{
  sqlite3_stmt *stmt;
  char *query = "UPDATE `meta` SET finish_time=?1 WHERE name=?2;";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_add_finish_time", "sqlite3_prepare_v2");
  }

  char *date = get_date();
  if (sqlite3_bind_text(stmt, 1, date, strlen(date), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_add_finish_time", "sqlite3_bind_text");
  }
  if (sqlite3_bind_text(stmt, 2, name, strlen(name), NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "meta_add_finish_time", "sqlite3_bind_text");
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file(db, "meta_add_finish_time", "sqlite3_step");
  }

  sqlite3_finalize(stmt);
}

/*
 * get_date returns date with current time
 */
char *get_date()
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char *date = malloc(sizeof(char) * 20);
  sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return date;
}

/*
 * write_error_to_log_file writes given error message to log file
 */
int write_error_to_log_file(sqlite3 *db, char *external_function, char *internal_function)
{
  char *log_path = getenv("NHI_LOG_PATH");
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

/*
 * get_latest_indicator gets latest shell indicator
 */
char *get_latest_indicator(sqlite3 *db)
{
  sqlite3_stmt *stmt;
  char *query = "SELECT name FROM `meta` ORDER BY rowid DESC LIMIT 1;";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file(db, "get_latest_indicator", "sqlite3_prepare_v2");
  }
  sqlite3_step(stmt);  /* No if check, because function complains for no reason, yet it works */

  char *indicator = malloc(11 * sizeof(char));
  strcpy(indicator, (char *)sqlite3_column_text(stmt, 0));
  sqlite3_finalize(stmt);
  return indicator;
}
