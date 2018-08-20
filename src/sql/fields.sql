-- Brian Chrzanowski
-- Sat Jul 21, 2018 00:40
--
-- stores the field values at a specific vertex

create table if not exists fields (
	field_id		integer primary key asc,
	run				integer not null,
	time_index		integer not null,

	vertex			integer null,
	e_x				real not null,
	e_y				real not null,
	e_z				real not null,
	b_x				real not null,
	b_y				real not null,
	b_z				real not null,

	foreign key(run) references run(run_id),
	foreign key(vertex) references vertexes(vertex_id)
);
