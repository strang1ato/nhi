#include <sqlite3.h>
#include <stddef.h>

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
