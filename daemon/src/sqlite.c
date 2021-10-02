#include "utils.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

sqlite3 *open_db(void);

void create_table(sqlite3 *, long);

void create_row(sqlite3 *, long);

void add_PS1(sqlite3 *, long, const void *);
void add_command(sqlite3 *, long, const void *);
void add_output(sqlite3 *, long, const void *, size_t);
void add_pwd(sqlite3 *, long, const void *);
void add_start_time(sqlite3 *, long);
void add_finish_time(sqlite3 *, long);
void add_indicator(sqlite3 *, long);

void meta_create_row(sqlite3 *, long);
void meta_add_finish_time(sqlite3 *, long);

sqlite3 *open_db(void)
{
  sqlite3 *db;
  char *db_path = "/var/nhi/db";
  if (sqlite3_open(db_path, &db) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
    return 0;
  }
  return db;
}

void create_table(sqlite3 *db, long indicator)
{
  sqlite3_stmt *stmt;
  char query[150];
  sprintf(query, "%s%ld%s",
          "CREATE TABLE `", indicator, "` (PS1 TEXT, command TEXT, output BLOB, pwd TEXT, start_time TEXT, finish_time TEXT, indicator INTEGER);");
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
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
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
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
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_bind_text(stmt, 1, PS1, strlen(PS1), NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
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
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_bind_text(stmt, 1, command, strlen(command), NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
}

void add_output(sqlite3 *db, long indicator, const void *output, size_t output_len)
{
  char query[100];
  sprintf(query, "%s%ld%s%ld%s",
          "UPDATE `", indicator, "` SET output=output || ? WHERE rowid = (SELECT MAX(rowid) FROM `", indicator, "`);");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_bind_blob(stmt, 1, output, output_len, NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
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
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_bind_text(stmt, 1, pwd, strlen(pwd), NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
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
    write_log(sqlite3_errmsg(db));
  }

  char *date = get_date();
  if (sqlite3_bind_text(stmt, 1, date, strlen(date), NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
}

void add_finish_time(sqlite3 *db, long indicator)
{
  char query[100];
  sprintf(query, "%s%ld%s%ld%s",
          "UPDATE `", indicator, "` SET finish_time=? WHERE rowid = (SELECT MAX(rowid) FROM `", indicator, "`);");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  char *date = get_date();
  if (sqlite3_bind_text(stmt, 1, date, strlen(date), NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
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
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_bind_int64(stmt, 1, command_indicator) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
}

void meta_create_row(sqlite3 *db, long indicator)
{
  sqlite3_stmt *stmt;
  char *query = "INSERT INTO `meta` VALUES (?1, ?2, ?3, NULL);";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_bind_int64(stmt, 1, indicator) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }
  char name[16];
  sprintf(name, "%ld", indicator);
  if (sqlite3_bind_text(stmt, 2, name, strlen(name), NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }
  char *date = get_date();
  if (sqlite3_bind_text(stmt, 3, date, strlen(date), NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
}

void meta_add_finish_time(sqlite3 *db, long indicator)
{
  sqlite3_stmt *stmt;
  char *query = "UPDATE `meta` SET finish_time=?1 WHERE indicator=?2;";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  char *date = get_date();
  if (sqlite3_bind_text(stmt, 1, date, strlen(date), NULL) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }
  if (sqlite3_bind_int64(stmt, 2, indicator) != SQLITE_OK) {
    write_log(sqlite3_errmsg(db));
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    write_log(sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
}
