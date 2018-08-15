-- Brian Chrzanowski
-- Sat Jul 21, 2018 00:40
--
-- table definition for particles
-- the implicit rowid for SQLite is used as a PK

create table if not exists particles (
	run				integer not null,
	particle_id		integer not null, -- simply the particle's number
	time_index		integer not null, -- number of time steps taken
	x_pos			real not null,
	y_pos			real not null,
	z_pos			real not null,
	x_vel			real not null,
	y_vel			real not null,
	z_vel			real not null,

	foreign key(run) references run(run_id)
);
