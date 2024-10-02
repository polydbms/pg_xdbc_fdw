-- here all tables should be created for the datasets, that will be read
--
-- native postgres_fdw does not need tables, they are just imported into schema native_fdw
--
-- jdbc tables need to be created with "jdbc." before the correct remote table name
-- the server has to be postgres_jdbc_server and the table schema has to match the remote schema
--
-- xdbc tables need to be created with "xdbc." before the table name and the correct remote table
-- name has to be in the options. The table schema has to match the schema .json file, that xdbc
-- fetches locally. The file has also to be added in the options. The schema files need to be present
-- in the container, so put them into the ressources/schemas/ folder before building the image or copy
-- them in afterward.
--
-- In general, put your tables below the already created tables and see their options.



--                              Create postgresql native fdw and import schemas
CREATE EXTENSION IF NOT EXISTS postgres_fdw;

CREATE SERVER postgres_server
    FOREIGN DATA WRAPPER postgres_fdw
    OPTIONS (host 'pg1', port '5432', dbname 'db1');

CREATE USER MAPPING FOR postgres
    SERVER postgres_server
    OPTIONS (user 'postgres', password '123456');

CREATE SCHEMA native_fdw;

IMPORT FOREIGN SCHEMA public
    FROM SERVER postgres_server
    INTO native_fdw;



--                                Create postgres jdbc fdw
CREATE EXTENSION jdbc_fdw;

CREATE SERVER postgres_jdbc_server FOREIGN DATA WRAPPER jdbc_fdw
    OPTIONS(
    drivername 'org.postgresql.Driver',
    url 'jdbc:postgresql://pg1:5432/db1',
    querytimeout '1200',
    jarfile '/pg_xdbc_fdw/ressources/driver/postgresql-42.7.4.jar'
    );

CREATE USER MAPPING FOR CURRENT_USER SERVER postgres_jdbc_server
    OPTIONS(username 'postgres',password '123456');

CREATE SCHEMA jdbc;

CREATE FOREIGN TABLE IF NOT EXISTS jdbc.pg1_sf0001_lineitem(
    l_orderkey      bigint not null,
    l_partkey       INTEGER,
    l_suppkey       INTEGER,
    l_linenumber    INTEGER not null,
    l_quantity      numeric(12,2),
    l_extendedprice numeric(12,2),
    l_discount      numeric(12,2),
    l_tax           numeric(12,2),
    l_returnflag    CHAR(1),
    l_linestatus    CHAR(1),
    l_shipdate      date,
    l_commitdate    date,
    l_receiptdate   date,
    l_shipinstruct  CHAR(25),
    l_shipmode      CHAR(10),
    l_comment       VARCHAR(44)
    ) SERVER postgres_jdbc_server;


CREATE FOREIGN TABLE IF NOT EXISTS jdbc.pg1_sf1_lineitem(
    l_orderkey      bigint not null,
    l_partkey       INTEGER,
    l_suppkey       INTEGER,
    l_linenumber    INTEGER not null,
    l_quantity      numeric(12,2),
    l_extendedprice numeric(12,2),
    l_discount      numeric(12,2),
    l_tax           numeric(12,2),
    l_returnflag    CHAR(1),
    l_linestatus    CHAR(1),
    l_shipdate      date,
    l_commitdate    date,
    l_receiptdate   date,
    l_shipinstruct  CHAR(25),
    l_shipmode      CHAR(10),
    l_comment       VARCHAR(44)
    ) SERVER postgres_jdbc_server;


CREATE FOREIGN TABLE IF NOT EXISTS jdbc.pg1_sf10_lineitem(
    l_orderkey      bigint not null,
    l_partkey       INTEGER,
    l_suppkey       INTEGER,
    l_linenumber    INTEGER not null,
    l_quantity      numeric(12,2),
    l_extendedprice numeric(12,2),
    l_discount      numeric(12,2),
    l_tax           numeric(12,2),
    l_returnflag    CHAR(1),
    l_linestatus    CHAR(1),
    l_shipdate      date,
    l_commitdate    date,
    l_receiptdate   date,
    l_shipinstruct  CHAR(25),
    l_shipmode      CHAR(10),
    l_comment       VARCHAR(44)
    ) SERVER postgres_jdbc_server;





--                                        Create xdbc_fdw extension
CREATE EXTENSION IF NOT EXISTS pg_xdbc_fdw;

-- Create dummy server (does not exist)
CREATE SERVER IF NOT EXISTS "xdbcserver"
    FOREIGN DATA WRAPPER pg_xdbc_fdw;

-- create schema
CREATE SCHEMA xdbc;

-- setup schemas for xdbc
CREATE FOREIGN TABLE IF NOT EXISTS xdbc.pg1_sf0001_lineitem(
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
    ) SERVER xdbcserver
    OPTIONS (schema_file_path '/pg_xdbc_fdw/ressources/schemas/lineitem_sf0001.json', server_host 'pg_xdbc_server',
        table 'pg1_sf0001_lineitem', buffer_size '1024', buffer_pool_size '32768');


CREATE FOREIGN TABLE IF NOT EXISTS xdbc.pg1_sf1_lineitem(
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
    ) SERVER xdbcserver
    OPTIONS (schema_file_path '/pg_xdbc_fdw/ressources/schemas/lineitem_sf0001.json', server_host 'pg_xdbc_server',
        table 'pg1_sf1_lineitem', buffer_size '1024', buffer_pool_size '32768');


CREATE FOREIGN TABLE IF NOT EXISTS xdbc.pg1_sf10_lineitem(
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
    ) SERVER xdbcserver
    OPTIONS (schema_file_path '/pg_xdbc_fdw/ressources/schemas/lineitem_sf0001.json', server_host 'pg_xdbc_server',
        table 'pg1_sf10_lineitem', buffer_size '1024', buffer_pool_size '32768');


-- Use all three schemas for querying
SET search_path TO jdbc, xdbc, native_fdw;
