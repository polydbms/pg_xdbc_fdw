EXTENSION = pg_xdbc_fdw
MODULE_big = pg_xdbc_fdw
DATA = pg_xdbc_fdw--0.1.sql

# add needed submodules
OBJS = src/pg_xdbc_fdw.o src/xdbc_client_interface.o src/xdbc_server_interface.o

PG_LIBS = -lpq
PG_CPPFLAGS = -I./include -I./submodules/xdbc-client -I./submodules/xdbc-server
PG_CFLAGS = -I./include

ifdef DEBUG
$(info $(shell echo "debug ist an"))
endif

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

#suppress warning with mixed declarations
$(OBJS): CFLAGS += $(PERMIT_DECLARATION_AFTER_STATEMENT)

PG_SHEET_FDW_AFTER_COMPILATION_TESTS:
