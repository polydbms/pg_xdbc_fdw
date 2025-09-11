DROP TABLE IF EXISTS jdbc.lineitem_sf10;
DROP TABLE IF EXISTS xdbc.lineitem_sf10;
CREATE FOREIGN TABLE IF NOT EXISTS jdbc.lineitem_sf10(
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

CREATE FOREIGN TABLE IF NOT EXISTS xdbc.lineitem_sf10(
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
    OPTIONS (schema_file_path '/pg_xdbc_fdw/ressources/schemas/lineitem_sf10.json', server_host 'xdbcserver',
        table 'lineitem_sf10', buffer_size '1024', buffer_pool_size '32768');