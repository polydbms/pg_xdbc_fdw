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
--                                        Create xdbc_fdw extension
CREATE EXTENSION IF NOT EXISTS pg_xdbc_fdw;

-- Create dummy server (does not exist)
CREATE SERVER IF NOT EXISTS "xdbcserver"
    FOREIGN DATA WRAPPER pg_xdbc_fdw;

-- create schema
CREATE SCHEMA xdbc;

-- Use all three schemas for querying
SET search_path TO jdbc, xdbc, native_fdw;
