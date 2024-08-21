//
// Created by Joel Ziegler on 09.08.2024
//

#ifndef PG_XDBC_FDW_XDBC_INTERFACE_H
#define PG_XDBC_FDW_XDBC_INTERFACE_H

// C interface for the fdw
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Struct holding all the options values possible for the xdbc client request.
     */
    typedef struct EnvironmentOptions {
        char *table;
        char *server_host;
        char *schema_file_with_path;
        int intermediate_format;
        int buffer_size;
        int bufferpool_size;
        int tuple_size;
        int sleep_time;
        int net_parallelism;
        int read_parallelism;
        int decomp_parallelism;
        int mode;
        long transfer_id;
    } EnvironmentOptions;

    /**
     * Struct for holding a buffer pointer and some metadata
     */
    typedef struct XdbcBuffer{
        int id;
        int iformat;
        unsigned long size;
        const unsigned char* data;
    } XdbcBuffer;

    /**
     * Creates an EnvironmentOptions struct with default values.
     * @return Returns an EnvironmentOptions struct with default values.
     */
    EnvironmentOptions createEnvOpt();

    /**
     * Mark a used buffer as read for the xclient. This invalidates the buffer, so that it can be reused.
     * @param transfer_id id of the current transfer under which the buffer was aquired
     * @param bufferID id of the buffer
     */
    void markXdbcBufferAsRead(int transfer_id, int bufferID);

    /**
     * Get the next buffer ready to be read for the assigned thread. A buffer has a plain bytes data pointer to the
     * tuples. The number of tuples per buffer and the size of all attributes in one tuple are listed in the RuntimeEnv.
     * Be careful! The last buffer may contain less tuples. So it is necessary to check the first attribute of each tuple
     * for existence to find the last buffer: int *firstAttribute = reinterpret_cast<int *>(dataPtr);

                    if (*firstAttribute < 0) {
                        spdlog::get("XCLIENT")->warn("Empty tuple at buffer: {0}, tupleNo: {1}", curBuffWithId.id, i);
                        break;
                    }
     * @param transfer_id id of the current transfer the reading thread belongs to
     * @param thr number of the current thread
     * @return XdbcBuffer ready to be read
     */
    XdbcBuffer getXdbcBuffer(int transfer_id, int thr);

    /**
     * Start an xclient with the given options. Connects the xclient with the given xserver from the options and starts
     * reading data from it.
     * @param envOpt Transfer target and options
     * @return Error code
     */
    int xdbcInitialize(EnvironmentOptions envOpt);

#ifdef __cplusplus
}
#endif


#endif // PG_XDBC_FDW_XDBC_INTERFACE_H