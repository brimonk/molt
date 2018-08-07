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

#include "sqlite3.h"
#include "particles.h"
#include "vect.h"

#define BUF_SIZE 2048

char *disk_to_mem(char *filename);

struct err_tbl {
	int sqlite_err;
	char *err_str;
};

extern char *io_db_tbls[]; /* use our sql file list outside of io.c */

/*
 * small, sqlite3 fatal error handler
 *
 * this function exits the program, regardless of issues because most SQLite
 * errors that AREN'T handled, will trickle to here. This simply prints an
 * error message, and quits.
 *
 * using the SQLITE3_ERR macro with a SQLITE3 return value should be all you
 * need, for useful debugging information
 */

enum {
	IO_DBWRAP_SELECT = 0x01,
	IO_DBWRAP_INSERT = 0x02,
	IO_DBWRAP_UPDATE = 0x04,
	IO_DBWRAP_DELETE = 0x08
};

#define DEFAULT_STRLEN 256

struct db_wrap_t {
	sqlite3 *db;
	char *sql;
	void *data;
	int (*func)(sqlite3_stmt *stmt, void *ptr);
	int op;
	int items;
	int item_size;
	int commit_size;
};


void io_erranddie(char *str, char *file, int line);
void sqlite3_wrap_errors(int val, char *file, int line, char *extra);
#define MEMORY_ERROR(a) (io_erranddie(a, __FILE__, __LINE__))
#define SQLITE3_ERR(a) (sqlite3_wrap_errors(a, __FILE__, __LINE__, NULL))
#define SQLITE3_ERR_EXTRA(a, b) (sqlite3_wrap_errors(a, __FILE__, __LINE__, b))

int io_dbwrap_do(struct db_wrap_t *wrapper);
int io_exec_sql_tbls(sqlite3 *db, char **tbl_list);
