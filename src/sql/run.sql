-- Brian Chrzanowski
-- Sat Jul 21, 2018 00:40
--
-- table definition with run information
-- this is also the "parent" table from which everything is defined

create table if not exists run (
	run_number		integer primary key asc,
	time_start		real not null,
	time_interval	real not null,
	time_stop		real not null
);
