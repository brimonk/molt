-- Brian Chrzanowski
-- Sat Jul 21, 2018 00:40
--
-- stores the field values at a specific vertex

create table if not exists fields (
	field_id		integer primary key asc,
	run				integer not null,
	time_index		integer not null,

	vertex			integer not null,
	electric_x		real not null,
	electric_y		real not null,
	electric_z		real not null,
	magnetic_x		real not null,
	magnetic_y		real not null,
	magnetic_z		real not null,

	foreign key(run) references run(run_id),
	foreign key(vertex) references vertexes(vertex_id)
);
