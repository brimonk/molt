## Small Usage Guide for MOLT

To avoid performance constraints and have a semi-consistent data structure
without excessive barriers to entry, the program logs all of its data
metrics to a [SQLite](https://www.sqlite.org/about.html) database.

Since SQLite competes with fopen, we avoid lots of kernel context swapping,
leaving as much of the program's execution in the program as possible. However,
there is a small issue, as now the application requires SQL to get data
out of the database, to be utilized by other applications.

For those wanting to use SQLite directly, there is an interface built for
Matlab, which can be found
[here](https://www.mathworks.com/help/database/ug/working-with-the-matlab-interface-to-sqlite.html).

## SQL Introduction for MOLT

For those people that have never used SQL before, this should be a small
introduction.

Structured Query Language, or SQL, organizes data into a series of ledgers.  SQL
looks very similarly to plain English. In fact, it's often very helpful when
coming up with a SQL query, to explain in an English sentence what you're trying
to get your database engine to do, then writing individual SQL statements to
achieve that. Typical SQL usage consists of 4 verbs, ``SELECT``, ``INSERT``,
``UPDATE``, and ``DELETE``. When defining your tables, ledgers as I alluded to
earlier, you use a ``CREATE`` statement, and in the event you'd like to remove
tables, they are ``DROP``ped from the database. Before we can delve into these
verbs, we first have to talk about how the data is organized.

### MOLT Data Structures

As of 2018-08-24, the data structures look like the following:

```
CREATE TABLE run (
	run_id			integer primary key asc,
	time_start		real not null,
	time_step		real not null,
	time_stop		real not null
);
CREATE TABLE particles (
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
CREATE TABLE fields (
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
CREATE TABLE vertexes (
	vertex_id		integer primary key asc,
	run				integer not null,
	xPos			real not null,
	yPos			real not null,
	zPos			real not null,

	foreign key(run) references run(run_id)
);
```

Defined in a hierarchical sense, the data structure looks something like this:

```
run
	particles

	vertexes
		fields
```

That is to say, run information is independent of particle data, field data, or
vertices.

Further discussing the data structure of the application, each particle is given
a particle_id, which is that particle's particular ID, a time index, or an
integer representing "where in time" this particle is, and position and velocity
vectors.

You'll notice, the bottom statement of the particle ``CREATE TABLE`` statement:

```
foreign key(run) references run(run_id)
```

This creates a constraint on the field "run". In the particle table, for each
entry, the run field's value must be a valid run_id within the run table. Using
this creates data integrity for how particles relate to run information.
Extending this to vertexes and fields, we can use this run_id to select all of
the information for a specific run, without delimiting in each table, but we'll
see that later.

Just keep in mind that this schema is for the program's requirements, and the
foreign keys are for data integrity.

The only other tricky portion of some tables is the constraint
"integer primary key asc" which does a few things. First, the field is defined
as an integer (go figure). Then, we state we'd like run_id to be a primary key,
meaning this field will be unique for each and every row. Finally, we state that
we'd like to always have it increase. This constraint will ensure that each
value is ascending (beginning at 1). Interestingly enough, if the run_id isn't
specified in an INSERT statement, the SQL engine will look at the most recent
row, and simply increment that number to get the new one.

### SELECT

As an introduction, let's first select all of the plain-jane particle
information. Just get everything in the table:

```
select * from particles;
```

As output, you should see something along the lines of this:

```
run         particle_id  time_index  x_pos             y_pos             z_pos             x_vel             y_vel             z_vel
----------  -----------  ----------  ----------------  ----------------  ----------------  ----------------  ----------------  ----------------
1           1            30          1051.75684954702  -442.10458093642  -1093.8150930148  44.0794343855602  58.9315124130489  11.7948208208172
1           2            30          578.821341528233  977.892113087046  -1049.1078091646  4.50182566256347  46.6401591816173  40.9452535104683
1           3            30          -120.87819169939  -153.53413284256  364.933129762244  61.6281867910308  4.86620098579032  39.3315973465012
1           4            30          -150.04821026561  334.508246983837  178.315877123058  34.4774657052371  4.58001035665162  38.8753811600038
1           5            30          328.411501153809  984.823864173026  -712.91202917924  11.0790132335755  34.9016883237761  46.7240476527829
...
```

Which is simply everything in the table.

This can be delimited by expressing what elements of the particle table you'd
like. For instance, if I just want particle_id, time_index, x_pos, x_vel, I
could write the following:

```
select particle_id, time_index, x_pos, x_vel from particles;
```

Which would only provide the following:


```
particle_id  time_index  x_pos             x_vel
-----------  ----------  ----------------  ---------------
0            0           1.72478604397028  22.636609773448
1            0           37.633211838842   9.2420985145690
2            0           37.7911608991172  30.362900181842
3            0           16.036120302992   53.288524384279
4            0           44.9767565340627  14.801336325146
...
```

Notice still, that this query is still providing every record in the
table; however, this doesn't delimit the rows, only the columns. To delimit
within the rows, a ``WHERE`` clause is needed. Imagining a table with multiple
runs, where ``run`` in the particle table is in {1, 2, 3, 4, 5...}, we can
delimit the following query by appending a where clause. The following where
clause will delimit the information to the "first" run.

```
select run, particle_id, time_index, x_pos, x_vel
from particles
where run = 1;
```

Which produces a table that looks like this:

```
run         particle_id  time_index  x_pos             x_vel
----------  -----------  ----------  ----------------  ---------------
1           0            0           1.72478604397028  22.636609773448
1           1            0           37.633211838842   9.2420985145690
1           2            0           37.7911608991172  30.362900181842
1           3            0           16.036120302992   53.288524384279
1           4            0           44.9767565340627  14.801336325146
...
(but run => 1 is the only value of run)
```

Those are the basics of ``SELECT`` statements. There are other clauses, like
``ORDER BY``, ``LIMIT``, and some more; however, the syntax for those are wildly
different depending on what database engine you use them in. Other things, like
[Common Table Expressions](https://www.sqlite.org/lang_with.html), have wildly
different syntaxes for each database engine, and the reader is encouraged to
look those up as they're needed. There may be examples in ``~/scripts``.

### INSERT

Insert statements provided to a database engine are parsed, and will ``INSERT``
a new row into the specified table. The syntax for SQLite can be found
[here](https://sqlite.org/lang_insert.html).

Showing some examples for an INSERT, the compiled MOLT program
executes the following to INSERT a new record into the run table:

```
insert into run (time_start, time_step, time_stop) values (?, ?, ?);
```

Woah! Lots to explain there.

The "insert into run" portion is fairly self-explanitory. We're simply inserting
something into the run table. What exactly those values are will be specified in
the following parenthesis surrounded parameters.

In this example, we're inserting the fields, ``time_start``, ``time_step``, and
``time_stop`` into run. If we look at the schema for the run table, something
interesting shows:

```
create table if not exists run (
	run_id                  integer primary key asc,
	time_start              real not null,
	time_step               real not null,
	time_stop               real not null
);
```

Which is the constraint pointed out in the CREATE table section. So, we needn't
include it in our INSERT statement: the database will handle it for us.

The field definition is then handled by a "values" clause, which specifies the
values the statement is INSERTing. These can be any values that match the type
of the specified columns. In this case, they all need to be REALs.

In MOLT, instead of spending time performing string concatenation to prepare up
a single SQL statement, then rinsing and repeating for each SQL statement,
there's a better solution. You can use 
[this interface](https://sqlite.org/c3ref/bind_blob.html) from the C interface
to substitute variables in the values clause.

Without it, inserting the default values would look something like this:

```
insert into run (time_start, time_step, time_stop) values (0, 0.5, 10);
```

### UPDATE

``UPDATE`` statements are similar in function to the ``INSERT`` statement in
that it directly alters what data is in the table. MOLT doesn't current use any
UPDATE statements, but suppose we'd like to alter some data after a simulation.
Saying we've found the simulation to be incorrect and all particles don't need
any X values, x_pos or x_vel. The following will update x_pos and x_vel to be
zero everywhere.

```
update particles set x_pos = 0, x_vel = 0;
```

Again, this kind of statement can be delimited with a ``WHERE`` clause.

### DELETE

``DELETE`` statements will remove records from a table. Simple enough.

```
DELETE from particles;
```

Will delete EVERYTHING from the particles table. This can be delimited to only
remove certain rows with a WHERE clause.

So, to only delete the particles that deal with the first run (run.run_id = 1),
the following statement could be used:

```
delete from particles where run = 1;
```

