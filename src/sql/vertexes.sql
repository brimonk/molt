-- Brian Chrzanowski
-- Sat Jul 21, 2018 16:37
--
-- table definition for grid vertexes
--
-- these form the one to many relationship present in the grids->vertexes
-- table. Using the link between field values, those field vertexes, and
-- each independent grid, it's possible to recompute much of what exists in
-- C, here.
--
-- As much of the problem is only defined for cubes (math hasn't been
-- discovered yet), it seems safe at this point to assume there are 8 vertexes
-- per "grid". If in the future, someone's discovered more boundary condition,
-- math, feel free to change the definition of the grid table to something like
-- 'polyhedrons'.
-- 
-- Another note here, we also store run_index absolutely everywhere. Not quite
-- sure why.

create table if not exists vertexes (
	run_index		integer not null,
	grid_id			integer not null,
	xPos			real not null,
	yPos			real not null,
	zPos			real not null,

	foreign key(run_index) references run(run_number)
);
