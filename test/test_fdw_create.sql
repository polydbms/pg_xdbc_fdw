-- Create fdw extension
CREATE EXTENSION IF NOT EXISTS pg_xdbc_fdw;

-- Create dummy server (does not exist)
-- The options could be important for the SheetReader
CREATE SERVER IF NOT EXISTS dummy
    FOREIGN DATA WRAPPER pg_xdbc_fdw;

-- No User mapping needed as its just a SheetReader

-- We need a schema for postgres, so it knows, what data to expect.
-- For that we create a Foreign table.
CREATE FOREIGN TABLE IF NOT EXISTS foreign_lineitem_sf0001(
    l_orderkey      INTEGER,
    l_partkey       INTEGER,
    l_suppkey       INTEGER,
    l_linenumber    INTEGER,
    l_quantity      double precision,
    l_extendedprice double precision,
    l_discount      double precision,
    l_tax           double precision,
    l_returnflag    CHAR(1),
    l_linestatus    CHAR(1),
    l_shipdate      VARCHAR(45),
    l_commitdate    VARCHAR(45),
    l_receiptdate   VARCHAR(45),
    l_shipinstruct  CHAR(26),
    l_shipmode      CHAR(11),
    l_comment       VARCHAR(45)
    ) SERVER dummy
    OPTIONS (schema_file_path '/pg_xdbc_fdw/ressources/schemas/lineitem_sf0001.json', server_host 'pg_xdbc_fdw-xdbc-server-1',
        table 'lineitem_sf0001');


select * from foreign_lineitem_sf0001;

-- Drop anything
DROP FOREIGN TABLE IF EXISTS foreign_lineitem_sf0001;
DROP SERVER IF EXISTS dummy;
DROP EXTENSION IF EXISTS pg_xdbc_fdw;