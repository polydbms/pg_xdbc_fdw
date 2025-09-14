#!/bin/bash
#set -x

# Set the timeout and table variables
TIMEOUT_DURATION=36000000
TABLE=$1
TRANSFER_ID=$2

# Execute the setup_fdw.sql file
psql db1 -f /pg_xdbc_fdw/experiments/setup_fdw.sql
psql db1 -f /tmp/setup_fdw_${TABLE}_${TRANSFER_ID}.sql

#psql db1 -v ON_ERROR_STOP=1 <<EOF
#\set TIMEOUT_DURATION $TIMEOUT_DURATION
#\set TABLE $TABLE

#SET statement_timeout = :TIMEOUT_DURATION;
#COPY (SELECT * FROM xdbc.:TABLE) TO '/tmp/out' WITH CSV HEADER;
#EOF
OUT="'/dev/null'"
#psql db1 -c "COPY (SELECT * FROM xdbc.${TABLE}) TO ${OUT} WITH CSV HEADER;"
psql db1 -c "SELECT COUNT(*) FROM xdbc.${TABLE}"