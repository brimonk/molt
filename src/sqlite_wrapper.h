/*
 * Brian Chrzanowski
 * Mon May 29, 2017 15:25
 *
 * A small SQLite3 wapper (with macros)
 *		for error handling, general messaging, etc
 */

struct err_tbl {
	int sqlite_err;
	char *err_str;
};

#define SQLITE3_ERR(a) (sqlite3_wrap_errors(a, __FILE__, __LINE__))
// #define SQLITE3_ERR(a) ( (a != SQLITE_OK) ? SQLITE3_ERR_INTERN(a) : )
void sqlite3_wrap_errors(int val, char *file, int line);
