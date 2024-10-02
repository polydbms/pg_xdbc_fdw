#!/bin/bash
set -x

# First, import tpch data as local tables in the server container. They are used in every benchmark query.
docker exec -u root pg1 bash -c "chmod +x import_tpch.sh"

docker exec pg1 bash -c "bash import_tpch.sh 1 1"
docker exec pg1 bash -c "bash create_indexes.sh 1 1"

docker exec pg1 bash -c "bash import_tpch.sh 1 0001"
docker exec pg1 bash -c "bash create_indexes.sh 1 0001"

docker exec pg1 bash -c "bash import_tpch.sh 1 10"
docker exec pg1 bash -c "bash create_indexes.sh 1 10"

# Secondly setup foreign tables and stuff in the client container
docker exec pg_xdbc_client bash -c "psql db1 -f /pg_xdbc_fdw/experiments/clean.sql"
docker exec pg_xdbc_client bash -c "psql db1 -f /pg_xdbc_fdw/experiments/setup_fdw.sql"