-- Brian Chrzanowski
-- Sat Jul 21, 2018 00:40
--
-- table definition for particles
-- the implicit rowid for SQLite is used as a PK

create table if not exists particles (
	run_id			integer not null,
	time_id			integer not null, -- number of time steps taken
	particle_id		integer not null, -- simply the particle's number
	x_pos			real not null,
	y_pos			real not null,
	z_pos			real not null,
	x_vel			real not null,
	y_vel			real not null,
	z_vel			real not null,

	foreign key(run_index) references run(run_number)
);
