//
// Created by joel on 14.08.24.
//

#include "Test_xdbc_interface.h"

void printBufferHead(XdbcBuffer buff, XdbcSchemaDesc desc) {
    printf("This buffer has id: %d, format: %d, tuplesCount: %ld, attributes per tuple: %ld\n", buff.id, buff.iformat,
           buff.tuplesCount, desc.attrCount);
}

void printBuffer(XdbcBuffer buff, XdbcSchemaDesc desc){
    unsigned char* dataPtr = buff.data;
    for(unsigned long i = 0; i < buff.tuplesCount; ++i){
        for(unsigned long j = 0; j < desc.attrCount; ++j){
            printf(" | ");
            switch (desc.attrTypeCodes[j]) {
                case 'S':
                    printf("%s", dataPtr);
                    break;
                case 'I':
                    printf("%d", *(int*)dataPtr);
                    break;
                case 'D':
                    printf("%f", *(double*)dataPtr);
                    break;
                case 'C':
                    printf("%c", *dataPtr);
                    break;
                default:
                    break;
            }
            dataPtr += desc.attrSizes[j];
        }
        printf("\n");
    }
}

void * threadFunction(void * arg){
    thread_params * params = (thread_params *)arg;
    printf("[%s] Started thread %d.\n", __func__, params->thread_num);
    XdbcBuffer buff;
    XdbcSchemaDesc desc = xdbcGetSchemaDesc(params->transfer_id);
    if(desc.attrCount == 0){
        printf("[%s] Thread %d: Schema desc with zero attributes!\n", __func__, params->thread_num);
        return NULL;
    }
    buff = xdbcGetBuffer(params->transfer_id, params->thread_num);
    while(buff.id >= 0){
        printBufferHead(buff, desc);
        // read just one buffer for testing
//        if(buff.id == 1) printBuffer(buff, desc);

        xdbcMarkBufferAsRead(params->transfer_id, buff.id);
        buff = xdbcGetBuffer(params->transfer_id, params->thread_num);
    }
    printf("[%s] Ended thread %d.\n", __func__, params->thread_num);
    return NULL;
}


int main(int argc, char** argv){
    printf("Start!\n");
    printf("Building RuntimeOptions...\n");

    XdbcEnvironmentOptions env = xdbcCreateEnvOpt();
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