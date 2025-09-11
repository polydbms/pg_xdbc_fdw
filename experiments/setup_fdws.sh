#!/bin/bash

psql -d db1 -f setup_fdw.sql
psql -d db1 -f setup_fdw_lineitem_sf10.sql
psql -d db1 -f setup_fdw_iotm.sql
psql -d db1 -f setup_fdw_inputeventsm.sql
psql -d db1 -f setup_fdw_ss13husallm.sql