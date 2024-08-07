/*---------------------------------------------------------------
 * foreign data-wrapper wrapping the xdbc client and server
 *
 * Original author: Joel Ziegler <cody14@freenet.de>
 *---------------------------------------------------------------
 */


CREATE FUNCTION pg_xdbc_fdw_handler()
RETURNS fdw_handler
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;
/*
CREATE FUNCTION pg-xdbc-fdw_validator(text[], oid)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;
*/
CREATE FOREIGN DATA WRAPPER pg_xdbc_fdw
  HANDLER pg_xdbc_fdw_handler
/*  VALIDATOR pg_xdbc_fdw_validator*/;
