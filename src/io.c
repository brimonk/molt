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

int gbl_autocommit = 0;

int io_store_parts(sqlite3 *db, struct molt_t *molt);
int io_store_fields(sqlite3 *db, struct molt_t *molt);
void io_toggleautocommit(sqlite3 *db);

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
	char *sql =
		"insert into run (time_start, time_step, time_stop) values" \
		"(?, ?, ?)";
	sqlite3_stmt *stmt;
	int rc;

	rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

	if (rc != SQLITE_OK)
		SQLITE3_ERR(db);

	/* bind parameters to the prepared sql, then execute */
	rc = sqlite3_bind_double(stmt, 1, molt->info.time_start);
	rc = sqlite3_bind_double(stmt, 2, molt->info.time_step);
	rc = sqlite3_bind_double(stmt, 3, molt->info.time_stop);

	/* execute the statement */
	rc = sqlite3_step(stmt);

	rc = sqlite3_finalize(stmt);

	return 0;
}

/* io_store_infots : write current timeslice of the problem into the database */
int io_store_infots(sqlite3 *db, struct molt_t *molt)
{
	if (io_store_parts(db, molt)) {
		return 1;
	}

	if (io_store_fields(db, molt)) {
		return 1;
	}

	return 0;
}

/* io_store_step : write particle information at the current timeslice */
int io_store_parts(sqlite3 *db, struct molt_t *molt)
{
	char *sql = "insert into particles (run, particle_id, time_index, " \
				"x_pos, y_pos, z_pos, x_vel, y_vel, z_vel) values " \
				"(?, ?, ?, ?, ?, ?, ?, ?, ?);";
	sqlite3_stmt *stmt;
	long i;
	int rc;

	sqlite3_exec(db, "BEGIN", 0, 0, 0);

	rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

	if (rc != SQLITE_OK)
		SQLITE3_ERR(db);

	for (i = 0; i < molt->part_total; i++) {
		/* bind parameters to the prepared sql, then execute */
		rc = sqlite3_bind_int(stmt,    1, molt->info.run_number);
		rc = sqlite3_bind_int64(stmt,  2, i);
		rc = sqlite3_bind_int(stmt,    3, molt->info.time_idx);
		rc = sqlite3_bind_double(stmt, 4, molt->parts[i].x);
		rc = sqlite3_bind_double(stmt, 5, molt->parts[i].y);
		rc = sqlite3_bind_double(stmt, 6, molt->parts[i].z);
		rc = sqlite3_bind_double(stmt, 7, molt->parts[i].vx);
		rc = sqlite3_bind_double(stmt, 8, molt->parts[i].vy);
		rc = sqlite3_bind_double(stmt, 9, molt->parts[i].vz);

		/* execute the statement */
		rc = sqlite3_step(stmt);

		/* reset the statement for a new loop iteration */
		rc = sqlite3_reset(stmt);
		rc = sqlite3_clear_bindings(stmt);
	}

	rc = sqlite3_finalize(stmt);

	sqlite3_exec(db, "COMMIT", 0, 0, 0);

	return 0;
}

/* io_store_fields : write fields information at the current timeslice */
int io_store_fields(sqlite3 *db, struct molt_t *molt)
{
	char *sql = "insert into fields "\
				"(run, time_index, e_x, e_y, e_z, b_x, b_y, b_z) values" \
				"(?, ?, ?, ?, ?, ?, ?, ?);";
	sqlite3_stmt *stmt;
	int rc;

	/* begin a transaction */
	sqlite3_exec(db, "BEGIN", 0, 0, 0);

	rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

	if (rc != SQLITE_OK)
		SQLITE3_ERR(db);

	/* bind parameters to the prepared sql, then execute */
	rc = sqlite3_bind_int(stmt,    1, molt->info.run_number);
	rc = sqlite3_bind_int(stmt,    2, molt->info.time_idx);
	rc = sqlite3_bind_double(stmt, 3, molt->e_field[0]);
	rc = sqlite3_bind_double(stmt, 4, molt->e_field[1]);
	rc = sqlite3_bind_double(stmt, 5, molt->e_field[2]);
	rc = sqlite3_bind_double(stmt, 6, molt->b_field[0]);
	rc = sqlite3_bind_double(stmt, 7, molt->b_field[1]);
	rc = sqlite3_bind_double(stmt, 8, molt->b_field[2]);

	/* execute the statement */
	rc = sqlite3_step(stmt);

	/* cleanup, cleanup, everybody everywhere */
	rc = sqlite3_finalize(stmt);

	sqlite3_exec(db, "COMMIT", 0, 0, 0);

	return 0;
}

int io_get_runid(sqlite3 *db, struct molt_t *molt)
{
	sqlite3_stmt *stmt;
	char *sql = "select max(run_id) from run;";
	int val, col;

	val = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (val != SQLITE_OK)
		SQLITE3_ERR(db);

	val = sqlite3_step(stmt);
	while (val == SQLITE_ROW) {
		col = sqlite3_column_int(stmt, 0);
		val = sqlite3_step(stmt);
	}

	if (val != SQLITE_DONE) {
		SQLITE3_ERR(db);
	}

	sqlite3_finalize(stmt);

	return col + 1; /* we want the "next" id */
}

void sqlite3_wrap_errors(sqlite3 *db, char *file, int line)
{
	fprintf(stderr, "%s:%d %s\n", file, line, sqlite3_errmsg(db));
	sqlite3_close(db);
	exit(1);
}
