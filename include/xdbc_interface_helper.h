//
// Created by joel on 19.08.24.
//

#ifndef PG_XDBC_FDW_XDBC_INTERFACE_HELPER_H
#define PG_XDBC_FDW_XDBC_INTERFACE_HELPER_H

#include <xclient.h>
#include <string>
#include <iostream>
#include <thread>
#include <numeric>
#include <iomanip>
#include <fstream>
#include <nlohmann/json.hpp>

#include <iostream>
#include <thread>
#include <numeric>
#include <fstream>
#include <iomanip>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
#include "xclient.h"
#include <algorithm>
#include <utility>


// debug flag. Uncomment to enable console debug printing
#define XDBCINTERFACE_DEBUG

#ifdef XDBCINTERFACE_DEBUG
#define debug_print(...) printf(__VA_ARGS__)
#else
#define debug_print(...) ((void) 0)
#endif


// Helper functions


// Helper Variables
std::map<long, std::shared_ptr<xdbc::XClient>> connectionsMap;
std::map<long, std::shared_ptr<xdbc::RuntimeEnv>> envMap;


#endif //PG_XDBC_FDW_XDBC_INTERFACE_HELPER_H
