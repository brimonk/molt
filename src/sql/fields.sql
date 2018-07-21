-- Brian Chrzanowski
-- Sat Jul 21, 2018 00:40
--
-- stores the field values at a specific vertex

create table if not exists fields (
	run_index		integer not null,
	time_index		integer not null,

	vertex			integer not null,
	electric_x		real not null,
	electric_y		real not null,
	electric_z		real not null,
	magnetic_x		real not null,
	magnetic_y		real not null,
	magnetic_z		real not null,

	-- do we need to store the overall forces at each vertex?
	-- no need to recompute

	foreign key(run_index) references run(run_number)
	foreign key(vertex) references run(vertexes)
);
