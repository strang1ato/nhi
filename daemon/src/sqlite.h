#include <sqlite3.h>
#include <stddef.h>

sqlite3 *open_db(void);

void create_table(sqlite3 *, long);

void create_row(sqlite3 *, long);

void add_PS1(sqlite3 *, long, void *);
void add_command(sqlite3 *, long, void *);
void add_output(sqlite3 *, long, void *, size_t);
void add_pwd(sqlite3 *, long, void *);
void add_start_time(sqlite3 *, long);
void add_finish_time(sqlite3 *, long);
void add_indicator(sqlite3 *, long);

void meta_create_row(sqlite3 *, long);
void meta_add_finish_time(sqlite3 *, long);
