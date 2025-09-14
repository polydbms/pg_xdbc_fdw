# Foreign Data Wrapper for XDBC Client and Server

This fdw interfaces with XDBC Client and XDBC Server, which establishes data transfer with PostgreSQL via XDBC.


## Instructions

### initialize submodules

```shell
git submodule update --init --recursive
```

### configure data

Check the tables/configurations in `experiments/setup_fdw*`

### build image

```shell
cd docker && make
```

### start container

```shell
docker compose -f docker-xdbc-fdw.yml up -d
```

### create table schemata

```shell
docker exec xdbcpostgres bash -c "cd pg_xdbc_fdw/experiments/ && ./setup_fdws.sh" 
```

### run experiments

```shell
./experiments/run_experiments_for_datasets.sh 1
```