//
// Created by joel on 14.08.24.
//

#ifndef PG_XDBC_FDW_TEST_XDBC_INTERFACE_H
#define PG_XDBC_FDW_TEST_XDBC_INTERFACE_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "xdbc_interface.h"

typedef struct{
    int thread_num;
    long transfer_id;
} thread_params;


#endif //PG_XDBC_FDW_TEST_XDBC_INTERFACE_H
