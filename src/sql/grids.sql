-- Brian Chrzanowski
-- Sat Jul 21, 2018 16:37
--
-- table definition for grids
--
-- each grid is really a collection of verticies

create table if not exists grids (
	run_index		integer not null,
	grid_index		integer not null,

	foreign key(run_index) references run(run_number)
);
