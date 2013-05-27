bench: bench.cc
	$(CXX) -g -std=c++0x $< -lpqxx -lboost_program_options -o $@

.PHONY: initdb
initdb:
	dropdb $(USER) || true
	createdb $(USER)
	psql -c "CREATE TABLE tags (id SERIAL PRIMARY KEY, primary_user INTEGER NOT NULL, user_ids INTEGER[] NOT NULL)"
	psql -c "CREATE INDEX ix_primary_user ON tags (primary_user)"

.PHONY: gin
gin: initdb bench
	psql -c "CREATE INDEX ix_user_ids ON tags USING GIN (user_ids)"

.PHONY: gist
gist: initdb bench
	psql -c "CREATE EXTENSION intarray" || true
	psql -c "CREATE INDEX ix_user_ids ON tags USING GIST (user_ids gist__int_ops)"

.PHONY: gistbig
gistbig: initdb bench
	psql -c "CREATE EXTENSION intarray" || true
	psql -c "CREATE INDEX ix_user_ids ON tags USING GIST (user_ids gist__intbig_ops)"

.PHONY: clean
clean:
	rm -f bench
