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

### SELECT

This is how you "get" data from a database. 
