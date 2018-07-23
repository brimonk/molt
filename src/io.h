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

#define SQLITE3_ERR(a) (sqlite3_wrap_errors(a, __FILE__, __LINE__, NULL))
#define SQLITE3_ERR_EXTRA(a, b) (sqlite3_wrap_errors(a, __FILE__, __LINE__, b))
void sqlite3_wrap_errors(int val, char *file, int line, char *extra);

int process_sql_tbls(sqlite3 *db, char **tbl_list);
