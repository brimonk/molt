/*
 * Brian Chrzanowski
 * Fri Jul 20, 2018 13:58
 *
 * input and output routines are defined here.
 *
 * Effectively, everything is performed through SQLite. Look at the ~/sql
 * directory to see the effective input and output routines. The database
 * should be prepared by calling, "io_db_prep" before any other routines are
 * called. This will execute all of the sql inside of the sql directory.
 */

#ifndef MOLT_IO
#define MOLT_IO

#include "sqlite3.h"
#include "vect.h"
#include "structures.h"

#define BUF_SIZE 2048

char *disk_to_mem(char *filename);

extern char *io_db_tbls[]; /* use our sql file list outside of io.c */

void sqlite3_wrap_errors(sqlite3 *db, char *file, int line);
#define MEMORY_ERROR(a) (io_erranddie(a, __FILE__, __LINE__))
#define SQLITE3_ERR(a) (sqlite3_wrap_errors(a, __FILE__, __LINE__))

void io_db_setup(sqlite3 *db);
int io_exec_sql_tbls(sqlite3 *db, char **tbl_list);
char *io_loadfile(char *filename);

int io_get_runid(sqlite3 *db, struct molt_t *molt);
int io_select_currentrunidx(sqlite3 *db);
int io_store_inforun(sqlite3 *db, struct molt_t *molt);
int io_store_infots(sqlite3 *db, struct molt_t *molt);

#endif
