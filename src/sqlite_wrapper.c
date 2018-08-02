/*
 * Brian Chrzanowski
 * Mon May 29, 2017 15:25
 *
 * A small SQLite3 wapper for error handling, general messaging, etc
 */

#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "sqlite_wrapper.h"

#define SQLITE_ERRFMT "%s::%d %s\n"

struct err_tbl error_lookups[] = {
	{SQLITE_OK, "How did you get here?"},
	{SQLITE_ERROR, "Generic SQLITE_ERROR returned"},
	{SQLITE_INTERNAL, "Internal logic error in SQLite"},
	{SQLITE_PERM, "Permission denied (check fs permissions)"},
	{SQLITE_ABORT, "callback routine requested an abort"},
	{SQLITE_BUSY, "the database file is locked"},
	{SQLITE_LOCKED, "a table in the database is locked"},
	{SQLITE_NOMEM, "a malloc() call failed"},
	{SQLITE_READONLY, "attempt to write a readonly database"},
	{SQLITE_INTERRUPT, "operation terminated by sqlite3_interrupt()"},
	{SQLITE_IOERR, "some sort of disk I/O error happened"},
	{SQLITE_CORRUPT, "the database disk image is malformed"},
	{SQLITE_NOTFOUND, "unknown opcode in sqlite3_file_control()"},
	{SQLITE_FULL, "insertion failed because the database is full"},
	{SQLITE_CANTOPEN, "unable to open the database file"},
	{SQLITE_PROTOCOL, "database lock protocol error"},
	{SQLITE_EMPTY, "... this is for internal (sqlite3) use only..."},
	{SQLITE_SCHEMA, "the schema has changed"},
	{SQLITE_TOOBIG, "string or blob exceeds size limit"},
	{SQLITE_CONSTRAINT, "abort due to constraint violation"},
	{SQLITE_MISMATCH, "data type mismatch"},
	{SQLITE_MISUSE, "library used incorrectly"},
	{SQLITE_NOLFS, "uses os features not supported on host"},
	{SQLITE_AUTH, "authorization denied"},
	{SQLITE_FORMAT, "... this isn't supposed to be used..."},
	{SQLITE_RANGE, "2nd parameter to sqlite3_bind out of range"},
	{SQLITE_NOTADB, "file opened that is not a database file"},
	{SQLITE_NOTICE, "notifications from sqlite3_log"},
	{SQLITE_WARNING, "warnings from sqlite3_log"},
	{SQLITE_ROW, "sqlite3_step has another row ready"},
	{SQLITE_DONE, "sqlite3_step has finished executing"}
};

void sqlite3_wrap_errors(int val, char *file, int line)
{
	int i;
	for (i = 0; i < sizeof(error_lookups) / sizeof(struct err_tbl); i++) {
		if (val == error_lookups[i].sqlite_err) {
			printf(SQLITE_ERRFMT, file, line, error_lookups[i].err_str);
			break;
		}
	}

	exit(1);
}

