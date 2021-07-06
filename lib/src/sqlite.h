#include <sqlite3.h>
#include <stddef.h>

sqlite3 *open_db();

void setup_queries(const char *);

void create_table(sqlite3 *, const char *);

void create_row(sqlite3 *);

void add_command(sqlite3 *, const char *, size_t);
void add_output(sqlite3 *, const char *);
void add_start_time(sqlite3 *);
void add_finish_time(sqlite3 *);
void add_indicator(sqlite3 *);

void meta_create_row(sqlite3 *, long, const char *);
void meta_add_finish_time(sqlite3 *, const char *);
