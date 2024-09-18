

#ifndef pg_xdbc_fdw_H
#define pg_xdbc_fdw_H


#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "foreign/fdwapi.h"
#include "foreign/foreign.h"
#include "nodes/nodes.h"
#include "optimizer/pathnode.h"
#include "optimizer/planmain.h"
#include "optimizer/restrictinfo.h"
#include "access/tupdesc.h"
#include "commands/defrem.h"

#include "utils/memutils.h"
#include "utils/builtins.h"
#include "utils/relcache.h"
#include "utils/date.h"
#include "utils/numeric.h"
#include "utils/rel.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/int8.h"

#include "catalog/pg_foreign_server.h"
#include "catalog/pg_foreign_table.h"
#include "catalog/pg_user_mapping.h"
#include "catalog/pg_type.h"

#include "nodes/nodes.h"
#include "nodes/makefuncs.h"
#include "nodes/pg_list.h"

#include "xdbc_interface.h"


// Debug mode flag. Uncomment to enable Postgresql debug output.
#define DEBUG

/* Macro to make conditional DEBUG more terse
 * Usage: elog(String); output can be found in Postgresql console */
#ifdef DEBUG
#define elog_debug(...) elog(NOTICE, __VA_ARGS__)
#else
#define elog_debug(...) ((void) 0)
#endif


// Initialization functions
extern void _PG_init(void);

extern void _PG_fini(void);


//=========================================================  FDW callback routines

void pg_xdbc_fdwBeginForeignScan(ForeignScanState *node, int eflags);

TupleTableSlot *pg_xdbc_fdwIterateForeignScan(ForeignScanState *node);

void pg_xdbc_fdwReScanForeignScan(ForeignScanState *node);

void pg_xdbc_fdwEndForeignScan(ForeignScanState *node);

void pg_xdbc_fdwGetForeignRelSize(PlannerInfo *root, RelOptInfo *baserel, Oid foreigntableid);

void pg_xdbc_fdwGetForeignPaths(PlannerInfo *root, RelOptInfo *baserel, Oid foreigntableid);

ForeignScan *
pg_xdbc_fdwGetForeignPlan(PlannerInfo *root, RelOptInfo *baserel, Oid foreigntableid, ForeignPath *best_path,
                           List *tlist, List *scan_clauses, Plan *outer_plan);

bool
pg_xdbc_fdwIsForeignScanParallelSafe(PlannerInfo *root, RelOptInfo *rel,
                          RangeTblEntry *rte);

Size
pg_xdbc_fdwEstimateDSMForeignScan(ForeignScanState *node, ParallelContext *pcxt);

void
pg_xdbc_fdwInitializeDSMForeignScan(ForeignScanState *node, ParallelContext *pcxt,
                         void *coordinate);

void
pg_xdbc_fdwReInitializeDSMForeignScan(ForeignScanState *node, ParallelContext *pcxt,
                           void *coordinate);

void
pg_xdbc_fdwInitializeWorkerForeignScan(ForeignScanState *node, shm_toc *toc,
                            void *coordinate);

void
pg_xdbc_fdwShutdownForeignScan(ForeignScanState *node);

//===========================================================  helper variables and structs

/**
 * Scanstate of the current scan. Used by the callback functions of this fdw.
 */
typedef struct {
    // current xclient buffer
    XdbcBuffer curbuff;
    unsigned long readTuples; // how many tuples already read from curbuf
    unsigned long totalReadTuples;
    XdbcSchemaDesc schemaDesc;
} pg_xdbc_scanstate;

/**
 * struct for one tuple slot. Has value and isnull fields.
 */
 typedef struct{
     Datum* values;
     bool* isnulls;
 } Fdw_one_slot;

//==============================================================  helper functions

/**
 * reads one tuple from the buffer and increments its read state.
 * @param buf buffer form which to read a tuple
 * @return Fdw_one_slot with value and isnull memory filled.
 */
Fdw_one_slot pg_xdbc_fdwReadTupleBuildSlot(pg_xdbc_scanstate * state);


/**
 * Parses fdw table options. One should create an EnvironmentOptions struct and pass pointers to the fields in here.
 * @param foreigntableid postgres provided id of the foreign table.
 * @param table
 * @param server_host
 * @param schema_file_path
 * @param iformat
 * @param buffer_size
 * @param buffer_pool_size
 * @param sleep_time
 * @param net_parallelism
 * @param read_parallelism
 * @param decomp_parallelism
 * @param tuple_size
 */
static void pg_xdbc_fdwGetOptions(Oid foreigntableid, char **table, char **server_host,
                                  char **schema_file_path, int* iformat, int* buffer_size, int* buffer_pool_size,
                                  int* sleep_time, int* net_parallelism, int* read_parallelism, int* decomp_parallelism,
                                  int* tuple_size);

/**
 * Retrieves a long from postgresql options element.
 * @param def
 * @return postgres long type
 */
int64
GetInt64Option(DefElem *def);

#endif //pg_xdbc_fdw_H