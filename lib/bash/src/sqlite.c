#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

char create_table_query[110],
     create_row_query[100],
     add_command_query[100],
     add_output_query[100],
     add_start_time_query[100],
     add_finish_time_query[100],
     add_indicator_query[100];

/*
 * setup_queries setups global variables containing reusable queries
 */
void setup_queries(const char *table_name)
{
  sprintf(create_table_query, "%s%s%s",
          "CREATE TABLE `", table_name, "` (command TEXT, output TEXT, start_time TEXT, finish_time TEXT, indicator INTEGER);");
  sprintf(create_row_query, "%s%s%s",
          "INSERT INTO `", table_name, "` VALUES (NULL, '', NULL, NULL, NULL);");
  sprintf(add_command_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET command=? WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  sprintf(add_output_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET output=output || ? WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  sprintf(add_start_time_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET start_time=? WHERE start_time is NULL AND rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  sprintf(add_finish_time_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET finish_time=? WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
  sprintf(add_indicator_query, "%s%s%s%s%s",
          "UPDATE `", table_name, "` SET indicator=? WHERE rowid = (SELECT MAX(rowid) FROM `", table_name, "`);");
}

/*
 * write_error_to_log_file writes given error message to log file
 */
int write_error_to_log_file(char *message)
{
  char *log_path = getenv("NHI_LOG_PATH");
  if (!log_path) {
    return EXIT_FAILURE;
  }
  FILE *log_fd = fopen(log_path, "a");
  fputs(message, log_fd);
  fclose(log_fd);
  return EXIT_SUCCESS;
}

/*
 * write_error_to_log_file writes given error message to log file
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
 * open_db opens nhi database
 */
sqlite3 *open_db()
{
  sqlite3 *db;
  char *db_path = getenv("NHI_DB_PATH");
  if (sqlite3_open(db_path, &db) != SQLITE_OK) {
    write_error_to_log_file("Operation of opening database failed\n");
  }
  return db;
}

/*
 * create_table with given name
 */
void create_table(sqlite3 *db, const char *table_name)
{
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, create_table_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_prepare_v2 function failed at create_table\n");
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file("sqlite3_step function failed at create_table\n");
  }
  sqlite3_finalize(stmt);
}

/*
 * create_row creates new empty row
 */
void create_row(sqlite3 *db, const char *table_name)
{
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, create_row_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_prepare_v2 function failed at create_row\n");
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file("sqlite3_step function failed at create_row\n");
  }
  sqlite3_finalize(stmt);
}

/*
 * add_command adds executed command to the last row
 */
void add_command(sqlite3 *db, const char *table_name, const void *command, size_t size)
{
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_command_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_prepare_v2 function failed at add_command\n");
  }
  if (sqlite3_bind_text(stmt, 1, command, size, NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_bind_text function failed at add_command\n");
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file("sqlite3_step function failed at add_command\n");
  }
  sqlite3_finalize(stmt);
}

/*
 * add_output adds output to the last row
 */
void add_output(sqlite3 *db, const char *table_name, const void *data)
{
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_output_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_prepare_v2 function failed at add_output\n");
  }
  if (sqlite3_bind_text(stmt, 1, data, strlen(data), NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_bind_text function failed at add_output\n");
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file("sqlite3_step function failed at add_output\n");
  }
  sqlite3_finalize(stmt);
}

/*
 * add_start_time adds start time to the last row
 */
void add_start_time(sqlite3 *db, const char *table_name)
{
  char *date = get_date();
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_start_time_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_prepare_v2 function failed at add_start_time\n");
  }
  if (sqlite3_bind_text(stmt, 1, date, strlen(date), NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_bind_text function failed at add_start_time\n");
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file("sqlite3_step function failed at add_start_time\n");
  }
  sqlite3_finalize(stmt);
}

/*
 * add_finish_time adds finish time to the last row
 */
void add_finish_time(sqlite3 *db, const char *table_name)
{
  char *date = get_date();
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_finish_time_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_prepare_v2 function failed at add_finish_time\n");
  }
  if (sqlite3_bind_text(stmt, 1, date, strlen(date), NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_bind_text function failed at add_finish_time\n");
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file("sqlite3_step function failed at add_finish_time\n");
  }
  sqlite3_finalize(stmt);
}

/*
 * add_indicator adds time in deciseconds since unix epoch to the last row
 */
void add_indicator(sqlite3 *db, const char *table_name)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  time_t seconds_part = now.tv_sec*10;
  suseconds_t deciseconds_part = now.tv_usec/100000;
  time_t indicator = seconds_part + deciseconds_part;
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, add_indicator_query, -1, &stmt, NULL) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_prepare_v2 function failed at add_indicator\n");
    printf("Error: %s\n", sqlite3_errmsg(db));
  }
  if (sqlite3_bind_int64(stmt, 1, indicator) != SQLITE_OK) {
    write_error_to_log_file("sqlite3_bind_int64 function failed at add_indicator\n");
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_error_to_log_file("sqlite3_step function failed at add_indicator\n");
  }
  sqlite3_finalize(stmt);
}
