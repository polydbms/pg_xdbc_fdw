//
// Created by joel on 14.08.24.
//

#include "Test_xdbc_interface.h"

void printBuffer(struct XdbcBuffer buff){
    printf("id: %d, iformat: %d, size: %lu\n", buff.id, buff.iformat, buff.size);
}

int main(int argc, char** argv){
    printf("Start!\n");
    printf("Building RuntimeOptions...\n");

    EnvironmentOptions env = createEnvOpt();
    // todo handle command line options to customize env
    env.table = "lineitem_sf0001";
    env.server_host = "pg_xdbc_fdw-xdbc-server-1";
    env.schema_file_with_path = "/pg_xdbc_fdw/ressources/schemas/lineitem_sf0001.json";

    printf("Calling xdbcInitialize...\n");

    int error = xdbcInitialize(env);

    if(error != 0){
        printf("Error at initialization!\n");
        return 1;
    }

    printf("Start reading buffers...\n");

    XdbcBuffer buff;
    buff = getXdbcBuffer(env.transfer_id, 0);
    while(buff.id >= 0){
        // todo read buffer
        printBuffer(buff);

        markXdbcBufferAsRead(env.transfer_id, buff.id);
        buff = getXdbcBuffer(env.transfer_id, 0);
    }

    printf("Finished!\n");
}