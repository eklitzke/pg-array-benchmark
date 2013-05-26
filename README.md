Introduction
-------------

This is a simple benchmark to test various strategies for using array
types with PostgreSQL. Here's a way to think about the
problem. Suppose we have a table of photos users uploaded, and users
can tag their friends. We want to do a query that will find all of the
photos a user uploaded *or* were tagged in by a friend.

The simplified table schema is like

```sql
CREATE TABLE tags (
    id SERIAL PRIMARY KEY,
    primary_user INTEGER NOT NULL,
    user_ids INTEGER[] NOT NULL
);
```

The `primary_user` is the one who uploaded the photo. There are four
possible query/indexing strategies we want to try.

Strategy 1
----------

Never insert the primary user into the `user_ids` array (only insert
the user ids for people they tagged), and the query is like

```sql
SELECT * FROM tags WHERE primary_user = ? OR user_ids @> ARRAY[?]
```

(where `?` is the single user id we're querying on).


Strategy 2
----------

Always insert the primary user into the `user_ids` array (in addition to
the user ids for people they tagged), and the query is like

```sql
SELECT * FROM tags WHERE user_ids @> ARRAY[?]
```

(where `?` is the single user id we're querying on).


Indexing Variation 1
--------------------

Index a GIN index on the `user_ids` column like:

```sql
CREATE INDEX ix_tags_user_ids ON tags USING GIN (user_ids);
```

Indexing Variation 2
--------------------

Index a GIST index on the `user_ids` column like:

```sql
CREATE EXTENSION intarray;
CREATE INDEX ix_tags_user_ids ON tags USING GIST (user_ids  gist__int_ops);
```

The Benchmark
--------------------

The benchmark is written in C++ using libpqxx. The main code for it
can be found in `bench.cc` and a benchmark runner is checked in at
`runall.sh`. There are various assumptions hard-coded into the
benchmark, so check out the source code.
