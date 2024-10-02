# Postgres Docker Image for PG_XDBC
This Docker Image compiles and tests the XDBC connector for Postgresql.

Use the Makefile for Image building. It generates one Image with a Postgresql 13 server and installs the XDBC Foreign Data Wrapper, which uses the XDBC client, and another Image with a Postgresql 13 server, a Clickhouse server and XDBC server installed. These can be used to experiment with data transfer between Postgresql 13 as client and files, Postgresql and Clickhouse as server.