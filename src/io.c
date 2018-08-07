/*
 * Brian Chrzanowski
 * Fri Jul 20, 2018 13:58
 *
 * input and output routines are defined here.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sqlite3.h"

#include "io.h"
#include "particles.h"

/*
 * The current plan, because it'll be really easy to extend later in time,
 * is to write generic-ish functions for select, insert, update, and delete.
 *
 * Each function will take a sqlite3 handle, a sql stmt to be run, a pointer
 * to an array of data, and a pointer to a function to execute to get
 * the raw form of that data.
 *
 * It's not the _best_, but shy of writing an ORM in a language that doesn't
 * have ORMs, it'll be fine.
 *
 * Another option is to wrap everything in a structure that looks something
 * like this
 *
 * struct db_wrap_t {
 * 		int debug; // optional
 * 		int op; // select = 1, insert = 2, update = 4, del = 8, other = err
 * 		char *sql;
 * 		void *data;
 * 		int datasize;
 * 		int (func *)(void *data, char **bufs)
 * };
 *
 * Where a pointer to this can then be passed to a single function, and
 * logic will flow to specific functions to do what I discussed above.
 */

static int io_dbwrap_select(struct db_wrap_t *wrapper);
static int io_dbwrap_insert(struct db_wrap_t *wrapper);
static int io_dbwrap_update(struct db_wrap_t *wrapper);
static int io_dbwrap_delete(struct db_wrap_t *wrapper);

/* this should be gathered dynamically from some rules somewhere */
char *io_db_tbls[] = 
{
	"src/sql/run.sql",
	"src/sql/particles.sql",
	"src/sql/fields.sql",
	"src/sql/vertexes.sql",
	NULL
};

#define SQLITE_ERRFMT "%s::%d %s\n"
#define SQLITE_ERRFMT_EXTRA "%s::%d %s EXTRA: %s\n"

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

int io_dbwrap_do(struct db_wrap_t *wrapper)
{
	int val;
	val = 0;

	switch (wrapper->op) {
	case IO_DBWRAP_SELECT:
		val = io_dbwrap_select(wrapper);
		break;

	case IO_DBWRAP_INSERT:
		val = io_dbwrap_insert(wrapper);
		break;

	case IO_DBWRAP_UPDATE:
		val = io_dbwrap_update(wrapper);
		break;

	case IO_DBWRAP_DELETE:
		val = io_dbwrap_delete(wrapper);
		break;

	default:
		// this also might be some sort of plain exec statement
		val = -1; // or some error code
		break;
	}

	return val;
}

static int io_dbwrap_select(struct db_wrap_t *w)
{
	sqlite3_stmt *stmt;
	int i, j, val;
	val = 0;

	// prep the statement
	val = sqlite3_prepare_v2(w->db, w->sql, -1, &stmt, NULL);
	if (val != SQLITE_OK) {
		SQLITE3_ERR(val);
	}

	/*
	 * now, we simply iterate over the 
	 */

	for (i = 0; i < w->items; i++) {
		for (j = 0; j < (w->commit_size > 0 ? w->commit_size : 1); j++) {
		}
	}

	return 0;
}

static int io_dbwrap_insert(struct db_wrap_t *wrapper)
{
	return 0;
}

static int io_dbwrap_update(struct db_wrap_t *wrapper)
{
	return 0;
}

static int io_dbwrap_delete(struct db_wrap_t *wrapper)
{
	return 0;
}

int io_exec_sql_tbls(sqlite3 *db, char **tbl_list)
{
	/* tbl_list is assumed to be NULL terminated */
	int i, val;
	char *file_buffer, *err;

	for (i = 0, val = 0; tbl_list[i] != NULL; i++) {
		file_buffer = disk_to_mem(tbl_list[i]);

		if (file_buffer) {
			/* execute the SQL */
			val = sqlite3_exec(db, file_buffer, NULL, NULL, &err);

			if (val != SQLITE_OK) {
				SQLITE3_ERR_EXTRA(val, err);
			} else {
				free(file_buffer);
			}

		} else {
			fprintf(stderr, "Error processing file: %s\n", tbl_list[i]);
		}
	}

	return val;
}


void sqlite3_wrap_errors(int val, char *file, int line, char *extra)
{
	int i;
	for (i = 0; i < sizeof(error_lookups) / sizeof(struct err_tbl); i++) {
		if (val == error_lookups[i].sqlite_err) {

			if (extra) {
				printf(SQLITE_ERRFMT_EXTRA,
						file, line, error_lookups[i].err_str, extra);
			} else {
				printf(SQLITE_ERRFMT, file, line, error_lookups[i].err_str);
			}

			break;
		}
	}

	exit(1); /* exit, returning '1' (error) to the environment */
}


char *disk_to_mem(char *filename)
{
	/* slurp up filename's contents to memory */
	FILE *fileptr;
	char *buffer;
	uint64_t filelen;

	buffer = NULL;
	filelen = 0;

	fileptr = fopen(filename, "r");

	if (fileptr) {
		/* jump from start to finish of a file to get its size */
		fseek(fileptr, 0, SEEK_END);
		filelen = ftell(fileptr);

		/* go back to the start */
		rewind(fileptr);

		buffer = malloc(filelen + 1); /* room for the file and a NULL byte */

		if (buffer) {
			memset(buffer, 0, filelen + 1);
			fread(buffer, filelen, 1, fileptr);
		}

		fclose(fileptr);
	}

	return buffer; /* caller frees the memory if non-NULL */
}


void io_erranddie(char *str, char *file, int line)
{
	fprintf(stderr, "%s:%d\t%s\n", file, line, str);
	exit(1);
}

