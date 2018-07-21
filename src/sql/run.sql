-- Brian Chrzanowski
-- Sat Jul 21, 2018 00:40
--
-- table definition with run information
-- the implicit rowid for SQLite is used as a PK
-- this is also the "parent" table from which everything is defined

create table if not exists run (
	run_number		integer primary key asc,
	time_start		integer not null,
	time_interval	integer not null,
	time_stop		integer not null
);
