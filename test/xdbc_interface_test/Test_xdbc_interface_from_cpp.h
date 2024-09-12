//
// Created by joel on 04.09.24.
//

#ifndef PG_XDBC_FDW_TEST_XDBC_INTERFACE_FROM_CPP_H
#define PG_XDBC_FDW_TEST_XDBC_INTERFACE_FROM_CPP_H

//#include "xdbc_interface.h"
//#include <iostream>
//#include <pthread.h>
//#include <unistd.h>
//#include <thread>
//#include <chrono>




#include <iostream>
#include <thread>
#include <numeric>
#include <fstream>
#include <iomanip>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
#include "../../submodules/xdbc-client/xdbc/xclient.h"
#include <algorithm>
#include <utility>

#include "xdbc_interface.h"

int read_parallelism;




#endif //PG_XDBC_FDW_TEST_XDBC_INTERFACE_FROM_CPP_H
