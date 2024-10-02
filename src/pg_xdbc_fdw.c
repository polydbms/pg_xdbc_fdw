
#include "pg_xdbc_fdw.h"

// Postgresql Magic block!
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

// Postgresql visible functions
PG_FUNCTION_INFO_V1(pg_xdbc_fdw_handler);
PG_FUNCTION_INFO_V1(pg_xdbc_fdw_validator);

// Gets called as soon as the library is loaded into memory once.
// Here you can load global stuff needed for the FDW to work.
void _PG_init(void){
    elog_debug("Initialization of FDW!");
    pg_xdbc_transfer_id = 0;
}

// Gets NEVER called!! (May change in postgres future)
void _PG_fini(void){
    elog_debug("deinitialization got called");
}

// The handler function just returns function pointers to all FDW functions
Datum pg_xdbc_fdw_handler(PG_FUNCTION_ARGS){
    elog_debug("[%s]",__func__);

    FdwRoutine *fdwroutine = makeNode(FdwRoutine);

    // base routines
    fdwroutine->GetForeignRelSize = pg_xdbc_fdwGetForeignRelSize;
    fdwroutine->GetForeignPaths = pg_xdbc_fdwGetForeignPaths;
    fdwroutine->GetForeignPlan = pg_xdbc_fdwGetForeignPlan;
    fdwroutine->BeginForeignScan = pg_xdbc_fdwBeginForeignScan;
    fdwroutine->IterateForeignScan = pg_xdbc_fdwIterateForeignScan;
    fdwroutine->ReScanForeignScan = pg_xdbc_fdwReScanForeignScan;
    fdwroutine->EndForeignScan = pg_xdbc_fdwEndForeignScan;

    // parallel routines
//    fdwroutine->IsForeignScanParallelSafe = pg_xdbc_fdwIsForeignScanParallelSafe;
//    fdwroutine->EstimateDSMForeignScan = pg_xdbc_fdwEstimateDSMForeignScan;
//    fdwroutine->InitializeDSMForeignScan = pg_xdbc_fdwInitializeDSMForeignScan;
//    fdwroutine->ReInitializeDSMForeignScan = pg_xdbc_fdwReInitializeDSMForeignScan;
//    fdwroutine->InitializeWorkerForeignScan = pg_xdbc_fdwInitializeWorkerForeignScan;
//    fdwroutine->ShutdownForeignScan = pg_xdbc_fdwShutdownForeignScan;

    PG_RETURN_POINTER(fdwroutine);
}

/*
 * Test whether a scan can be performed within a parallel worker.
 * This function will only be called when the planner believes that a parallel plan might be possible,
 * and should return true if it is safe for that scan to run within a parallel worker.
 * This will generally not be the case if the remote data source has transaction semantics,
 * unless the worker's connection to the data can somehow be made to share the same transaction context as the leader.
 * If this function is not defined, it is assumed that the scan must take place within the parallel leader.
 * Note that returning true does not mean that the scan itself can be done in parallel,
 * only that the scan can be performed within a parallel worker. Therefore,
 * it can be useful to define this method even when parallel execution is not supported.
 */
bool
pg_xdbc_fdwIsForeignScanParallelSafe(PlannerInfo *root, RelOptInfo *rel,
                          RangeTblEntry *rte){
    elog_debug("%s",__func__);
    return false;
}

/*
 * Estimate the amount of dynamic shared memory that will be required for parallel operation.
 * This may be higher than the amount that will actually be used, but it must not be lower.
 * The return value is in bytes. This function is optional, and can be omitted if not needed;
 * but if it is omitted, the next three functions must be omitted as well,
 * because no shared memory will be allocated for the FDW's use.
 */
Size
pg_xdbc_fdwEstimateDSMForeignScan(ForeignScanState *node, ParallelContext *pcxt){
    elog_debug("%s",__func__);
    return 0;
}

/*
 * Initialize the dynamic shared memory that will be required for parallel operation.
 * coordinate points to a shared memory area of size equal to the return value of EstimateDSMForeignScan.
 * This function is optional, and can be omitted if not needed.
 */
void
pg_xdbc_fdwInitializeDSMForeignScan(ForeignScanState *node, ParallelContext *pcxt,
                         void *coordinate){
    elog_debug("%s",__func__);
}

/*
 * Re-initialize the dynamic shared memory required for parallel operation when the
 * foreign-scan plan node is about to be re-scanned. This function is optional, and
 * can be omitted if not needed. Recommended practice is that this function reset only shared state,
 * while the ReScanForeignScan function resets only local state. Currently,
 * this function will be called before ReScanForeignScan, but it's best not to rely on that ordering.
 */
void
pg_xdbc_fdwReInitializeDSMForeignScan(ForeignScanState *node, ParallelContext *pcxt,
                           void *coordinate){
    elog_debug("%s",__func__);
}

/*
 * Initialize a parallel worker's local state based on the shared state set up by the
 * leader during InitializeDSMForeignScan. This function is optional, and can be omitted if not needed.
 */
void
pg_xdbc_fdwInitializeWorkerForeignScan(ForeignScanState *node, shm_toc *toc,
                            void *coordinate){
    elog_debug("%s",__func__);
}
/*
 * Release resources when it is anticipated the node will not be executed to completion.
 * This is not called in all cases; sometimes, EndForeignScan may be called without this
 * function having been called first. Since the DSM segment used by parallel query is destroyed
 * just after this callback is invoked, foreign data wrappers that wish to take some action
 * before the DSM segment goes away should implement this method.
 */
void
pg_xdbc_fdwShutdownForeignScan(ForeignScanState *node){
    elog_debug("%s",__func__);
}

/*
 * This function should update baserel->rows to be the expected number of rows returned by the table scan,
 * after accounting for the filtering done by the restriction quals.
 * The initial value of baserel->rows is just a constant default estimate, which should be replaced if at all possible.
 * The function may also choose to update baserel->width if it can compute a better estimate of the average result row width.
 * (The initial value is based on column data types and on column average-width values measured by the last ANALYZE.)
 * Also, this function may update baserel->tuples if it can compute a better estimate of the foreign table's total row count.
 * (The initial value is from pg_class.reltuples which represents the total row count seen by the last ANALYZE.)
 */
void pg_xdbc_fdwGetForeignRelSize(PlannerInfo *root, RelOptInfo *baserel, Oid foreigntableid){
    XdbcEnvironmentOptions* envOpt = palloc(sizeof(XdbcEnvironmentOptions));
    XdbcEnvironmentOptions tempOpt = xdbcCreateEnvOpt();
    memcpy(envOpt, &(tempOpt), sizeof(XdbcEnvironmentOptions ));
    pg_xdbc_fdwGetOptions(foreigntableid, &envOpt->table, &envOpt->server_host,
                          &envOpt->schema_file_with_path, &envOpt->mode,
                          &envOpt->buffer_size, &envOpt->bufferpool_size,
                          &envOpt->sleep_time, &envOpt->net_parallelism, &envOpt->read_parallelism,
                          &envOpt->decomp_parallelism, &envOpt->tuple_size);

    baserel->rows = (double) 1000;

    // store options for later?
    List *fdw_private;
    fdw_private = list_make1(envOpt);
    baserel->fdw_private = fdw_private;
}

/*
 * This function must generate at least one access path (ForeignPath node) for a scan on the foreign table
 * and must call add_path to add each such path to baserel->pathlist.
 * It's recommended to use create_foreignscan_path to build the ForeignPath nodes.
 * The function can generate multiple access paths, e.g., a path which has valid pathkeys to represent a pre-sorted result.
 * Each access path must contain cost estimates,
 * and can contain any FDW-private information that is needed to identify the specific scan method intended.
 */
void pg_xdbc_fdwGetForeignPaths(PlannerInfo *root, RelOptInfo *baserel, Oid foreigntableid){
    elog_debug("[%s]",__func__);

    Cost startup_cost = 0;
    Cost total_cost = baserel->rows;
    List *fdw_private = baserel->fdw_private;
    add_path(baserel,(Path *) create_foreignscan_path(root, baserel, NULL, baserel->rows, startup_cost, total_cost, NIL, NULL, NULL, fdw_private));
}

/*
 * Create a ForeignScan plan node from the selected foreign access path. This is called at the end of query planning.
 * The parameters are as for GetForeignRelSize, plus the selected ForeignPath
 * (previously produced by GetForeignPaths, GetForeignJoinPaths, or GetForeignUpperPaths),
 * the target list to be emitted by the plan node, the restriction clauses to be enforced by the plan node,
 * and the outer subplan of the ForeignScan, which is used for rechecks performed by RecheckForeignScan.
 * (If the path is for a join rather than a base relation, foreigntableid is InvalidOid.)
 *
 * This function must create and return a ForeignScan plan node;
 * it's recommended to use make_foreignscan to build the ForeignScan node.
 */
ForeignScan* pg_xdbc_fdwGetForeignPlan(PlannerInfo *root, RelOptInfo *baserel, Oid foreigntableid, ForeignPath *best_path, List *tlist, List *scan_clauses, Plan *outer_plan){
    elog_debug("[%s]", __func__);

    // Just copied!!
    Index scan_relid = baserel->relid;
    scan_clauses = extract_actual_clauses(scan_clauses, false);
    List* fdw_private = baserel->fdw_private;
    return make_foreignscan(tlist, scan_clauses, scan_relid, NIL, fdw_private, NIL, NIL, NULL);
}

/*
 * Begin executing a foreign scan. This is called during executor startup.
 * It should perform any initialization needed before the scan can start,
 * but not start executing the actual scan (that should be done upon the first call to IterateForeignScan).
 * The ForeignScanState node has already been created, but its fdw_state field is still NULL.
 * Information about the table to scan is accessible through the ForeignScanState node
 * (in particular, from the underlying ForeignScan plan node, which contains any FDW-private information provided by GetForeignPlan).
 * eflags contains flag bits describing the executor's operating mode for this plan node.
 */
void pg_xdbc_fdwBeginForeignScan(ForeignScanState *node, int eflags){

    // Retrieve EnvironmentOptions
    List* fdw_private = ((ForeignScan *)node->ss.ps.plan)->fdw_private;
    XdbcEnvironmentOptions* envOpt = linitial(fdw_private);
    envOpt->transfer_id = ++pg_xdbc_transfer_id;

    elog_debug("[%s] Initialize scan with options:\n  table: %s\n  server-host: %s\n"
               "  schema_file_path: %s\n  transfer_id: %lu\n", __func__ , envOpt->table, envOpt->server_host,
               envOpt->schema_file_with_path, envOpt->transfer_id);

    // Initilize xclient connection to server
    int error = xdbcInitialize(*envOpt);

    if(error) {
        ereport(ERROR, (errcode(ERRCODE_FDW_ERROR), errmsg("Failed to initialize XClient! Error code: %d" , error)));
    }

    // build scan state
    elog_debug("[%s] Building scan state...", __func__);
    pg_xdbc_scanstate *state = (pg_xdbc_scanstate *) palloc(sizeof(pg_xdbc_scanstate));
    state->curbuff = xdbcGetBuffer(envOpt->transfer_id, 0);
    state->schemaDesc = xdbcGetSchemaDesc(envOpt->transfer_id);
    state->readTuples = 0;
    state->totalReadTuples = 0;

    // store scan state pointer
    elog_debug("[%s] Finished scan setup.", __func__);
    node->fdw_state = (void*) state;
}

pg_xdbc_slot pg_xdbc_fdwReadTupleBuildSlot(pg_xdbc_scanstate * state){
    unsigned char* dataPtr = state->curbuff.data;
    dataPtr += state->readTuples * state->schemaDesc.rowOffset;
    pg_xdbc_slot slot;
    slot.values = (Datum *) palloc(sizeof(Datum) * state->schemaDesc.attrCount);
    slot.isnulls = (bool *) palloc(sizeof(bool*) * state->schemaDesc.attrCount);
    for(unsigned long i = 0; i < state->schemaDesc.attrCount; ++i){
        slot.isnulls[i] = false;
        switch (state->schemaDesc.attrTypeCodes[i]) {
            case 'S':
                ;
                char *s = (char*)dataPtr;
                slot.values[i] = CStringGetTextDatum(s);
                break;
            case 'I':
                slot.values[i] = Int32GetDatum(*(int*)dataPtr);
                break;
            case 'D':
                slot.values[i] = Float8GetDatum(*(double*)dataPtr);
                break;
            case 'C':
                ;
                char c = *(char*)dataPtr;
                slot.values[i] = CStringGetTextDatum(&c);
                break;
            default:
                slot.isnulls[i] = true;
                break;
        }
        dataPtr += state->schemaDesc.attrSizes[i];
    }
    return slot;
}


/*
 * Fetch one row from the foreign source, returning it in a tuple table slot
 * (the node's ScanTupleSlot should be used for this purpose). Return NULL if no more rows are available.
 * The tuple table slot infrastructure allows either a physical or virtual tuple to be returned;
 * in most cases the latter choice is preferable from a performance standpoint.
 * Note that this is called in a short-lived memory context that will be reset between invocations.
 * Create a memory context in BeginForeignScan if you need longer-lived storage, or use the es_query_cxt of the node's EState.
 */
TupleTableSlot *pg_xdbc_fdwIterateForeignScan(ForeignScanState *node){

    /*
     * retrieve scanstate with current active buffer
     * take next tuple from it
     * if empty fetch next buffer from xclient
     */

    // retrieve scanstate and environment
    pg_xdbc_scanstate *state = node->fdw_state;
    List* fdw_private = ((ForeignScan *)node->ss.ps.plan)->fdw_private;
    XdbcEnvironmentOptions* envOpt = linitial(fdw_private);

    // check if all tuples read
    if(state->readTuples >= state->curbuff.tuplesCount){
        // get new buffer
        //elog_debug("[%s] Mark old buffer as read", __func__);
        xdbcMarkBufferAsRead(envOpt->transfer_id, state->curbuff.id);
        state->curbuff = xdbcGetBuffer(envOpt->transfer_id, 0);
        //elog_debug("[%s] Got new buffer with %lu tuples", __func__, state->curbuff.tuplesCount);
        state->readTuples = 0;
        // check if all buffers read
        if(state->curbuff.id < 0){
            elog_debug("[%s] All buffers finished. Total amount of tuples read: %lu", __func__, state->totalReadTuples);
            return NULL;
        }
    }

    pg_xdbc_slot newslot = pg_xdbc_fdwReadTupleBuildSlot(state);

    // build tuple from already fetched rows and increment read counter
//    elog_debug("[%s] Storing tuple in TupleTableSlot from batch %lu at index %lu", __func__, state->batchIndex-1, state->rowsRead );
    TupleTableSlot *slot = node->ss.ss_ScanTupleSlot;
    ExecClearTuple(slot);
    // virtual tuple
    slot->tts_values = newslot.values;
    slot->tts_isnull = newslot.isnulls;
    ExecStoreVirtualTuple(slot);
    // heap tuple
//    Datum* values = &(state->cells[state->batchIndex-1][state->rowsRead * state->columnCount]);
//    bool* isnull = &(state->isnull[state->batchIndex-1][state->rowsRead * state->columnCount]);
//    HeapTuple tuple = heap_form_tuple(slot->tts_tupleDescriptor, values, isnull);
//    ExecStoreHeapTuple(tuple, slot, 0);
    state->readTuples++;
    state->totalReadTuples++;
    return slot;
}

/*
 * Restart the scan from the beginning. Note that any parameters the scan depends on may have changed value,
 * so the new scan does not necessarily return exactly the same rows.
 */
void pg_xdbc_fdwReScanForeignScan(ForeignScanState *node){elog_debug("%s",__func__);}

/*
 * End the scan and release resources. It is normally not important to release palloc'd memory,
 * but for example open files and connections to remote servers should be cleaned up.
 *
 * we release our palloced memory from our own memory context. it is probably hierarchically under the context
 * of the whole foreign table scan and will be released with it. But we can already release it here.
 */
void pg_xdbc_fdwEndForeignScan(ForeignScanState *node){
    elog_debug("[%s] Closing connection...", __func__);

    List* fdw_private = ((ForeignScan *)node->ss.ps.plan)->fdw_private;
    XdbcEnvironmentOptions* envOpt = linitial(fdw_private);
    xdbcClose(envOpt->transfer_id);
    pg_xdbc_scanstate *state = node->fdw_state;
    pfree(state);
}

/*
 * Read an options value and convert it to long. Throw error if it can't.
 */
int64
GetInt64Option(DefElem *def)
{
    int64 result;
    char *str_val = defGetString(def);

    if (!scanint8(str_val, true, &result))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                        errmsg("invalid value for option \"%s\": \"%s\"",
                               def->defname, str_val)));
    return result;
}


/*
 * Fetches values from OPTIONS in Foreign Server and Table registration.
 */
static void
pg_xdbc_fdwGetOptions(Oid foreigntableid, char **table, char **server_host,
                      char **schema_file_path, int* iformat, int* buffer_size, int* buffer_pool_size,
                      int* sleep_time, int* net_parallelism, int* read_parallelism, int* decomp_parallelism,
                      int* tuple_size)
{
    ForeignTable	*f_table;
    ForeignServer	*f_server;
    List		*options;
    ListCell	*lc;

    /*
     * Extract options from FDW objects.
     */
    f_table = GetForeignTable(foreigntableid);
    f_server = GetForeignServer(f_table->serverid);

    options = NIL;
    options = list_concat(options, f_table->options);
    options = list_concat(options, f_server->options);

    bool foundTable = false;
    bool foundSchemaPath = false;
    bool foundServerHost = false;

    /* Loop through the options, and get the values */
    foreach(lc, options)
    {
        DefElem *def = (DefElem *) lfirst(lc);

        if (strcmp(def->defname, "table") == 0)
        {
            *table = defGetString(def);
            foundTable = true;
            elog_debug("[%s] Got table with value: %s", __func__, *table);
        }

        if (strcmp(def->defname, "server_host") == 0)
        {
            *server_host = defGetString(def);
            foundServerHost = true;
            elog_debug("[%s] Got server_host with value: %s", __func__, *server_host);
        }

        if (strcmp(def->defname, "schema_file_path") == 0)
        {
            *schema_file_path = defGetString(def);
            foundSchemaPath = true;
            elog_debug("[%s] Got schema_file_path with value: %s", __func__, *schema_file_path);
        }

        if (strcmp(def->defname, "iformat") == 0)
        {
            char *value_str = defGetString(def);
            *iformat = pg_strtoint32(value_str);
            elog_debug("[%s] Got iformat with value: %d", __func__, *iformat);
        }

        if (strcmp(def->defname, "buffer_size") == 0)
        {
            char *value_str = defGetString(def);
            int customBufferSize = pg_strtoint32(value_str);
            if (customBufferSize > 0)
            {
                *buffer_size = customBufferSize;
                elog_debug("[%s] Got buffer_size with value: %d", __func__, *buffer_size);
            }
        }

        if (strcmp(def->defname, "buffer_pool_size") == 0)
        {
            char *value_str = defGetString(def);
            int customBufferPoolSize = pg_strtoint32(value_str);
            if (customBufferPoolSize > 0)
            {
                *buffer_pool_size = customBufferPoolSize;
                elog_debug("[%s] Got buffer_pool_size with value: %d", __func__, *buffer_pool_size);
            }
        }

        if (strcmp(def->defname, "sleep_time") == 0)
        {
            char *value_str = defGetString(def);
            int customSleepTime = pg_strtoint32(value_str);
            if (customSleepTime > 0)
            {
                *sleep_time = customSleepTime;
                elog_debug("[%s] Got sleep_time with value: %d", __func__, *sleep_time);
            }
        }

        if (strcmp(def->defname, "net_parallelism") == 0)
        {
            char *value_str = defGetString(def);
            int customNetParallelism = pg_strtoint32(value_str);
            if (customNetParallelism > 0)
            {
                *net_parallelism = customNetParallelism;
                elog_debug("[%s] Got net_parallelism with value: %d", __func__, *net_parallelism);
            }
        }

        if (strcmp(def->defname, "read_parallelism") == 0)
        {
            char *value_str = defGetString(def);
            int customReadParallelism = pg_strtoint32(value_str);
            if (customReadParallelism > 0)
            {
                *read_parallelism = customReadParallelism;
                elog_debug("[%s] Got read_parallelism with value: %d", __func__, *read_parallelism);
            }
        }

        if (strcmp(def->defname, "decomp_parallelism") == 0)
        {
            char *value_str = defGetString(def);
            int customDecompParallelism = pg_strtoint32(value_str);
            if (customDecompParallelism > 0)
            {
                *decomp_parallelism = customDecompParallelism;
                elog_debug("[%s] Got decomp_parallelism with value: %d", __func__, *decomp_parallelism);
            }
        }

        if (strcmp(def->defname, "tuple_size") == 0)
        {
            char *value_str = defGetString(def);
            int customTupleSize = pg_strtoint32(value_str);
            if (customTupleSize > 0)
            {
                *tuple_size = customTupleSize;
                elog_debug("[%s] Got tuple_size with value: %d", __func__, *tuple_size);
            }
        }
    }


    if(!foundSchemaPath) ereport(ERROR,
                           (errcode(ERRCODE_FDW_OPTION_NAME_NOT_FOUND),
                                   errmsg("missing mandatory option \"schema_file_path\"" )));

    if(!foundTable) ereport(ERROR,
                                 (errcode(ERRCODE_FDW_OPTION_NAME_NOT_FOUND),
                                         errmsg("missing mandatory option \"table\"" )));

    if(!foundServerHost) ereport(ERROR,
                                 (errcode(ERRCODE_FDW_OPTION_NAME_NOT_FOUND),
                                         errmsg("missing mandatory option \"server_host\"" )));
}
