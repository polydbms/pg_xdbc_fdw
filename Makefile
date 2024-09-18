EXTENSION = pg_xdbc_fdw
MODULE_big = pg_xdbc_fdw
DATA = pg_xdbc_fdw--0.1.sql

# add object files and needed submodules to compile
OBJS = src/pg_xdbc_fdw.o src/xdbc_interface.o

# c and cpp prepocessor flags
PG_CPPFLAGS = -I./include -I./submodules/xdbc-client/xdbc -I./submodules/xdbc-server

# cpp flags
PG_CXXFLAGS = -std=c++17 -pthread

# linker flags, used by PG when linking the .o files with gcc
#PG_LDFLAGS = -pthread

# link flag when specifically linking a shared library
SHLIB_LINK = -lfmt -lxdbc -lspdlog -lboost_program_options -pthread -lpq

# c only flags
PG_CFLAGS = -I./include -pthread

# !! THERE IS NO FLAG FOR LLVM IR GENERATION. For example -std=c++17 is needed here, so the .cpp files need
# to be precompiled to .bc files manually with the required flag.

ifdef DEBUG
$(info $(shell echo "debug ist an"))
endif

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

#suppress warning with mixed declarations
$(OBJS): CFLAGS += $(PERMIT_DECLARATION_AFTER_STATEMENT)

PG_XDBC_FDW_AFTER_COMPILATION_TESTS:
