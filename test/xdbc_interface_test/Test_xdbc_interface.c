//
// Created by joel on 14.08.24.
//

#include "Test_xdbc_interface.h"


void printBuffer(XdbcBuffer buff){
    printf("id: %d, iformat: %d, size: %lu\n", buff.id, buff.iformat, buff.tuplesCount);
}

void * threadFunction(void * arg){
    thread_params * params = (thread_params *)arg;
    printf("[%s] Started thread %d.\n", __func__, params->thread_num);
    XdbcBuffer buff;
    buff = getXdbcBuffer(params->transfer_id, params->thread_num);
    while(buff.id >= 0){
        // todo read buffer
        printBuffer(buff);

        markXdbcBufferAsRead(params->transfer_id, buff.id);
        buff = getXdbcBuffer(params->transfer_id, params->thread_num);
    }
    printf("[%s] Ended thread %d.\n", __func__, params->thread_num);
    return NULL;
}


int main(int argc, char** argv){
    printf("Start!\n");
    printf("Building RuntimeOptions...\n");

    EnvironmentOptions env = createEnvOpt();
    // todo handle command line options to customize env
    env.table = "lineitem_sf0001";
    env.server_host = "pg_xdbc_fdw-xdbc-server-1";
    env.schema_file_with_path = "/pg_xdbc_fdw/ressources/schemas/lineitem_sf0001.json";

    env.net_parallelism = 4;
    env.read_parallelism = 4;

    printf("Calling xdbcInitialize...\n");

    int error = xdbcInitialize(env);

    if(error){
        printf("Error at initialization!\n");
        return 1;
    }

    printf("Start reading buffers...\n");

    pthread_t threads[env.read_parallelism];  // Declare thread identifier
    thread_params threadParams[env.read_parallelism];

    for(int i = 0; i < env.read_parallelism; ++i){
        threadParams[i].transfer_id = env.transfer_id;
        threadParams[i].thread_num = i;

        // Create a thread and execute the threadFunction
        if (pthread_create(&threads[i], NULL, threadFunction, (void*)&(threadParams[i])) != 0) {
            printf( "Error creating thread %d\n", i);
            return 1;
        }
    }

    printf("Waiting for threads.\n");
    for(int i = 0; i < env.read_parallelism; ++i){
        // Wait for the thread to finish execution
        if (pthread_join(threads[i], NULL) != 0) {
            printf( "Error joining thread %d\n", i);
            return 2;
        }
    }

    printf("All threads finished, closing connection!\n");

    xdbcClose(env.transfer_id);

    printf("Finished!\n");
}