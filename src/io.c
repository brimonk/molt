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
#include <assert.h>

#include "sqlite3.h"
#include "io.h"

int io_store_parts(sqlite3 *db, struct molt_t *molt);
int io_store_fields(sqlite3 *db, struct molt_t *molt);

/* this should be gathered at run time from some rules somewhere */
char *io_db_tbls[] = 
{
	"src/sql/run.sql",
	"src/sql/particles.sql",
	"src/sql/fields.sql",
	"src/sql/vertexes.sql",
	NULL
};

int io_exec_sql_tbls(sqlite3 *db, char **tbl_list)
{
	/* tbl_list is assumed to be NULL terminated */
	int i, val;
	char *file_buffer, *err;

	for (i = 0, val = 0; tbl_list[i] != NULL; i++) {
		file_buffer = io_loadfile(tbl_list[i]);

		if (file_buffer) {
			/* execute the SQL */
			val = sqlite3_exec(db, file_buffer, NULL, NULL, &err);

			if (val != SQLITE_OK) {
				SQLITE3_ERR(db);
			} else {
				free(file_buffer);
			}

		} else {
			fprintf(stderr, "Error processing file: %s\n", tbl_list[i]);
		}
	}

	return val;
}


void io_db_setup(sqlite3 *db)
{
	sqlite3_exec(db, "pragma journal_mode=wal;", 0, 0, 0);
}

/* io_loadfile : load the contents of 'filename' into buffer and return it */
char *io_loadfile(char *filename)
{
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

int io_select_currentrunidx(sqlite3 *db)
{
	int val, col;
	sqlite3_stmt *stmt;
	char *sql =
		"select max(run_id) from run;";

	val = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (val != SQLITE_OK) {
		SQLITE3_ERR(db);
	}

	val = sqlite3_step(stmt);

	while (val == SQLITE_ROW) {
		col = sqlite3_column_int(stmt, 0);
		val = sqlite3_step(stmt);
	}

	if (val != SQLITE_DONE) {
		fprintf(stderr, "%s\n", sqlite3_errstr(val));
		SQLITE3_ERR(db);
	}

	sqlite3_finalize(stmt);

	return col;
}

/* io_store_inforun : writes constant run information */
int io_store_inforun(sqlite3 *db, struct molt_t *molt)
{
	return 0;
}

/* io_store_infots : write current timeslice of the problem into the database */
int io_store_infots(sqlite3 *db, struct molt_t *molt)
{
	int val;

	val = 0;

	val += io_store_parts(db, molt);
	val += io_store_fields(db, molt);

	return val;
}

/* io_store_step : write particle information at the current timeslice */
int io_store_parts(sqlite3 *db, struct molt_t *molt)
{
	return 0;
}

/* io_store_fields : write fields information at the current timeslice */
int io_store_fields(sqlite3 *db, struct molt_t *molt)
{
	return 0;
}

void sqlite3_wrap_errors(sqlite3 *db, char *file, int line)
{
	fprintf(stderr, "%s:%d %s\n", file, line, sqlite3_errmsg(db));
	sqlite3_close(db);
	exit(1);
}
