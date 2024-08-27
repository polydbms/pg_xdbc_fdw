//
// Created by joel on 14.08.24.
//

#include "Test_xdbc_interface.h"

void printBuffer(XdbcBuffer buff){
    printf("id: %d, iformat: %d, size: %lu\n", buff.id, buff.iformat, buff.size);
}

void * threadFunction(void * arg){
    printf("[%s] Started thread.\n", __func__);
    unsigned long* transfer_id = (unsigned long*)arg;
    XdbcBuffer buff;
    buff = getXdbcBuffer(*transfer_id, 0);
    while(buff.id >= 0){
        // todo read buffer
        //printBuffer(buff);
        printf(".\n");

        markXdbcBufferAsRead(transfer_id, buff.id);
        buff = getXdbcBuffer(transfer_id, 0);
    }
    printf("[%s] Ended thread.\n", __func__);
}


int main(int argc, char** argv){
    printf("Start!\n");
    printf("Building RuntimeOptions...\n");

    EnvironmentOptions env = createEnvOpt();
    // todo handle command line options to customize env
    env.table = "lineitem_sf10";
    env.server_host = "pg_xdbc_fdw-xdbc-server-1";
    env.schema_file_with_path = "/pg_xdbc_fdw/ressources/schemas/lineitem_sf0001.json";

    printf("Calling xdbcInitialize...\n");

    int error = xdbcInitialize(env);

    if(error != 0){
        printf("Error at initialization!\n");
        return 1;
    }

    printf("Start reading buffers...\n");
    sleep(1);
    printf("now\n");

    pthread_t thread;  // Declare a thread identifier

    // Create a thread and execute the threadFunction
    if (pthread_create(&thread, NULL, threadFunction, (void*)&(env.transfer_id)) != 0) {
        printf( "Error creating thread\n");
        return 1;
    }

    printf("Waiting for thread.\n");

    // Wait for the thread to finish execution
    if (pthread_join(thread, NULL) != 0) {
        printf( "Error joining thread\n");
        return 2;
    }

    printf("Finished!\n");
}