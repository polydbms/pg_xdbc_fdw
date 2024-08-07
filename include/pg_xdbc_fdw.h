

#ifndef pg_sheet_fdw_H
#define pg_sheet_fdw_H


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


// Debug mode flag. Uncomment to enable Postgresql debug output.
//#define DEBUG

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


//========================  Custom c++ functions and structs

/**
 * Enum type for the C++ Sheetreader Cell Enum
 */
enum PGExcelCellType {
    T_NONE = 0, // blank cell
    T_NUMERIC = 1, // integer or double
    T_STRING_REF = 2, // static string reference. Get C string from readStaticString().
    T_STRING = 3, // multi referenced string. Get C string from readDynamicString().
    T_STRING_INLINE = 4, // multi referenced string. Get C string from readDynamicString().
    T_BOOLEAN = 5, // boolean represented as int
    T_ERROR = 6, // Sheetreader error value
    T_DATE = 7 // datetime value, already as unix timestamp (seconds since 1970), Excel stores as number of days since 1900
};

/**
 * Cell Struct for the C++ Sheetreader Cell Struct
 */
struct PGExcelCell {
    union {
        double real;
        unsigned long long stringIndex;
        unsigned char boolean;  // Using char (1 byte) for boolean in C
    } data;
    unsigned char type;
};

/**
 * Register one Sheet from an Excel file. This lets Sheetreader initialize its reading. Memory gets filled with data from the Sheet.
 * @param pathToFile The absolute path of the Excel file. Mandatory.
 * @param sheetName The name of the Sheet to read. Empty string defaults to the first Sheet.
 * @param tableOID User chosen id for the Sheet. Needed in all follow up methods to reference the Sheet. Can be the tableOID of the foreign table Postgresql assigns.
 * @param numberOfThreads Number of Threads Sheetreader uses for parsing the Sheet. -1 lets Sheetreader chose a sane value.
 * @return Row count of the Excel Sheet.
 */
unsigned long registerExcelFileAndSheetAsTable(const char *pathToFile, const char *sheetName, unsigned int tableOID, int numberOfThreads);

/**
 * Prepares the next row to be read. Returns the number of columns of the row.
 * @param tableOID The ID of the Sheet to be referenced. Is set by user in registerExcelFileAndSheetAsTable().
 * @return The number of columns of the row.
 */
unsigned long startNextRow(unsigned int tableOID);

/**
 * Fetches the next cell in the row. Returns a full copied struct of the cell.
 * @param tableOID The ID of the Sheet to be referenced. Is set by user in registerExcelFileAndSheetAsTable().
 * @return PGExcelCell with cell value.
 */
struct PGExcelCell getNextCell(unsigned int tableOID);

/**
 * Fetches the next cell in the row. Returns a pointer of the cell.
 * @param tableOID The ID of the Sheet to be referenced. Is set by user in registerExcelFileAndSheetAsTable().
 * @return PGExcelCell pointer with cell value.
 */
struct PGExcelCell *getNextCellCast(unsigned int tableOID);

/**
 * Reads a static string from the Excel string array.
 * @param tableOID The ID of the Sheet to be referenced. Is set by user in registerExcelFileAndSheetAsTable().
 * @param stringIndex Index of the string to be read.
 * @return C string
 */
char* readStaticString(unsigned int tableOID, unsigned long long stringIndex);

/**
 * Reads a dynamic (multi referenced) string from the Excel string array.
 * @param tableOID The ID of the Sheet to be referenced. Is set by user in registerExcelFileAndSheetAsTable().
 * @param stringIndex Index of the string to be read.
 * @return C string
 */
char* readDynamicString(unsigned int tableOID, unsigned long long stringIndex);

/**
 * Drop all ressources of one Sheet.
 * @param tableOID The ID of the Sheet to be referenced. Is set by user in registerExcelFileAndSheetAsTable().
 */
void dropTable(unsigned int tableOID);

//=========================================================  FDW callback routines

void pg_sheet_fdwBeginForeignScan(ForeignScanState *node, int eflags);

TupleTableSlot *pg_sheet_fdwIterateForeignScan(ForeignScanState *node);

void pg_sheet_fdwReScanForeignScan(ForeignScanState *node);

void pg_sheet_fdwEndForeignScan(ForeignScanState *node);

void pg_sheet_fdwGetForeignRelSize(PlannerInfo *root, RelOptInfo *baserel, Oid foreigntableid);

void pg_sheet_fdwGetForeignPaths(PlannerInfo *root, RelOptInfo *baserel, Oid foreigntableid);

ForeignScan *
pg_sheet_fdwGetForeignPlan(PlannerInfo *root, RelOptInfo *baserel, Oid foreigntableid, ForeignPath *best_path,
                           List *tlist, List *scan_clauses, Plan *outer_plan);


//===========================================================  helper variables and structs

/**
 * Scanstate of the current scan. Used by the callback functions of this fdw.
 */
typedef struct {
    // Memory context for allocations
    MemoryContext context;
    // used as unique identifier in the parserInterface
    unsigned int tableID;
    int columnCount;
    // size of one prefetch batch
    int batchSize;
    // how many rows not read
    unsigned long rowsLeft;
    // how many rows already fetched
    unsigned long rowsPrefetched;
    // how many rows already read
    unsigned long rowsRead;
    // index of the currently used batch
    unsigned long batchIndex;
    // Used to check against received types from the ParserInterface. Essentially the schema of the foreign table.
    Oid* expectedTypes;
    // memory for prefetched values
    Datum** cells;
    bool** isnull;
} pg_sheet_scanstate;

//==============================================================  helper functions

/**
 * Parse foreign table options.
 * @param foreigntableid id of the foreign table, of which the options should be parsed.
 * @param filepath Absolute path of the Excel File.
 * @param sheetname Name of the Sheet to be read. Defaults to first Sheet.
 * @param batchSize Number of rows to prefetch. Defaults to a size that leads to 101 batches.
 * @param numberOfThreads Number of threads Sheetreader uses for Sheet parsing. Defaults to a sane value based on current system.
 * @param skipRows Number of header rows to skip. Defaults to 0.
 */
static void pg_sheet_fdwGetOptions(Oid foreigntableid, char **filepath, char **sheetname, unsigned long* batchSize, int* numberOfThreads, int* skipRows);

/**
 * Converts a numerical value from Sheetreader to the expected type of Postgresql.
 * @param cell Cell with value to convert.
 * @param expectedType Postgresql type id, that describes the type to convert to.
 * @return Datum with the converted type.
 */
Datum pg_sheet_fdwConvertSheetNumericToPG(struct PGExcelCell* cell, Oid expectedType);

#endif //pg_sheet_fdw_H