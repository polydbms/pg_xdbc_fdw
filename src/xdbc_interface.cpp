//
// Created by Joel Ziegler on 09.08.2024
//

#include "xdbc_interface_helper.h"
#include "xdbc_interface.h"

void xdbcMarkBufferAsRead(long transfer_id, int bufferID){
    auto it = connectionsMap.find(transfer_id);
    if (it != connectionsMap.end()) {
        auto xclient = it->second;
        xclient->markBufferAsRead(bufferID);
    }
}

XdbcBuffer xdbcGetBuffer(long transfer_id, int thr){
    try {
//        debug_print("[%s]\n", __func__);
        auto it = connectionsMap.find(transfer_id);
        if (it != connectionsMap.end()) {
            auto xclient = it->second;
            while (xclient->hasNext(thr)) {
//                debug_print("[%s] Waiting for next buffer...\n", __func__);
                xdbc::buffWithId curBuffWithId = xclient->getBuffer(thr);
//                debug_print("[%s] Got next buffer...\n", __func__);

                if (curBuffWithId.id >= 0) {
                    auto dataPtr = curBuffWithId.buff;
                    XdbcBuffer buff{
                            curBuffWithId.id,
                            curBuffWithId.iformat,
                            static_cast<unsigned long>(curBuffWithId.totalTuples),
                            reinterpret_cast<unsigned char *>(dataPtr)
                    };
                    return buff;
                } else {
                    debug_print("[%s] found invalid buffer! id: %d\n", __func__,
                                curBuffWithId.id);
                    break;
                }
            }
        } else {
            debug_print("[%s] Couldn't find xclient_int for transfer_id: %ld\n", __func__, transfer_id );
        }
    } catch (const std::exception& e) {
        // Catch all standard exceptions derived from std::exception
        debug_print("[%s] Standard exception caught: %s\n", __func__, e.what());
    } catch (...) {
        // Catch any other types of exceptions
        debug_print("[%s] Unknown exception caught!\n", __func__);
    }


    debug_print("[%s] No more buffers.\n", __func__);

    XdbcBuffer buff2{
            -1,
            -1,
            0,
            nullptr
    };
    return buff2;
}

XdbcSchemaDesc xdbcGetSchemaDesc(long transfer_id){
    auto it = envMap.find(transfer_id);
    if (it != envMap.end()) {
        auto env = it->second;
        XdbcSchemaDesc schemaDesc;

        schemaDesc.attrCount = env->schema.size();
        auto* sizes = (unsigned long *)malloc(schemaDesc.attrCount * sizeof(unsigned long));
        auto* typeCodes = (unsigned long *)malloc(schemaDesc.attrCount * sizeof(unsigned long));
        auto* inRowOffsets = (unsigned long *)malloc(schemaDesc.attrCount * sizeof(unsigned long));
        schemaDesc.rowOffset = 0;

        for (unsigned long i = 0; i < schemaDesc.attrCount; ++i) {
            inRowOffsets[i] = schemaDesc.rowOffset;
            sizes[i] = env->schema[i].size;
            typeCodes[i] = static_cast<unsigned long>(static_cast<unsigned char>(env->schema[i].tpe[0]));
            schemaDesc.rowOffset += sizes[i];
        }
        schemaDesc.attrSizes = sizes;
        schemaDesc.attrTypeCodes = typeCodes;
        schemaDesc.inRowOffsets = inRowOffsets;
        return schemaDesc;
    } else {
        XdbcSchemaDesc error;
        error.attrCount = 0;
        return error;
    }
}

XdbcEnvironmentOptions xdbcCreateEnvOpt(){
    XdbcEnvironmentOptions envOpt;
    envOpt.intermediate_format = 1;
    envOpt.buffer_size = 64;
    envOpt.bufferpool_size = 4096;
    envOpt.sleep_time = 5;
    envOpt.mode = 1;
    envOpt.net_parallelism = 1;
    envOpt.read_parallelism = 1;
    envOpt.decomp_parallelism = 1;
    envOpt.transfer_id = 0;
    envOpt.table = nullptr;
    envOpt.server_host = nullptr;
    envOpt.schema_file_with_path = nullptr;
    envOpt.tuple_size = 0;
    return envOpt;
}

void initializeEnv(const std::shared_ptr<xdbc::RuntimeEnv>& envi, XdbcEnvironmentOptions &envOpt){
    debug_print("[%s]\n", __func__);

    // assign transaction parameters
    envi->table = envOpt.table;
    envi->server_host = envOpt.server_host;
    envi->iformat = envOpt.intermediate_format;
    envi->buffer_size = envOpt.buffer_size;
    envi->buffers_in_bufferpool = envOpt.bufferpool_size / envOpt.buffer_size;
    envi->tuple_size = envOpt.tuple_size;
    envi->sleep_time = std::chrono::milliseconds(envOpt.sleep_time);
    envi->rcv_parallelism = envOpt.net_parallelism;
    envi->write_parallelism = envOpt.read_parallelism;
    envi->decomp_parallelism = envOpt.decomp_parallelism;
    envi->mode = envOpt.mode;
    envi->transfer_id = envOpt.transfer_id;

    // assign static values
    envi->schemaJSON = "";
    envi->server_port = "1234";

    envi->tuple_size = 0;
    envi->tuples_per_buffer = 0;
    envi->monitor.store(false);
}

std::string formatSchema(const std::vector<xdbc::SchemaAttribute>& schema) {
    std::stringstream ss;

    // Header line
    ss << std::setw(20) << std::left << "Name"
       << std::setw(15) << std::left << "Type"
       << std::setw(10) << std::left << "Size"
       << '\n';

    for (const auto &tuple: schema) {
        ss << std::setw(20) << std::left << tuple.name
           << std::setw(15) << std::left << tuple.tpe
           << std::setw(10) << std::left << tuple.size
           << '\n';
    }

    return ss.str();
}

using namespace std;
namespace po = boost::program_options;

vector<xdbc::SchemaAttribute> createSchemaFromConfig(const string &configFile) {
    ifstream file(configFile);
    if (!file.is_open()) {
        debug_print("Failed to open schema: %s\n", configFile.c_str());
    }
    nlohmann::json schemaJson;
    file >> schemaJson;

    vector<xdbc::SchemaAttribute> schema;
    for (const auto &item: schemaJson) {
        schema.emplace_back(xdbc::SchemaAttribute{
                item["name"],
                item["type"],
                item["size"]
        });
    }
    return schema;
}

std::string readJsonFileIntoString(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        debug_print("Failed to open schema: %s\n", filePath.c_str());
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

// Function to print EnvironmentOptions using debug_print
void printEnvironmentOptions(const XdbcEnvironmentOptions& envOptions) {
    debug_print("Environment Options:\n");
    debug_print("  Table: %s\n", (envOptions.table ? envOptions.table : "null"));
    debug_print("  Server Host: %s\n", (envOptions.server_host ? envOptions.server_host : "null"));
    debug_print("  Schema File Path: %s\n", (envOptions.schema_file_with_path ? envOptions.schema_file_with_path : "null"));
    debug_print("  Intermediate Format: %d\n", envOptions.intermediate_format);
    debug_print("  Buffer Size: %d\n", envOptions.buffer_size);
    debug_print("  Bufferpool Size: %d\n", envOptions.bufferpool_size);
    debug_print("  Tuple Size: %d\n", envOptions.tuple_size);
    debug_print("  Sleep Time: %d\n", envOptions.sleep_time);
    debug_print("  Net Parallelism: %d\n", envOptions.net_parallelism);
    debug_print("  Read Parallelism: %d\n", envOptions.read_parallelism);
    debug_print("  Decomp Parallelism: %d\n", envOptions.decomp_parallelism);
    debug_print("  Mode: %d\n", envOptions.mode);
    debug_print("  Transfer ID: %ld\n", envOptions.transfer_id);
}

// Function to print RuntimeEnv using debug_print
void printRuntimeEnv(const xdbc::RuntimeEnv& runtimeEnv) {
    debug_print("Runtime Environment:\n");
    debug_print("  Transfer ID: %ld\n", runtimeEnv.transfer_id);
    debug_print("  Buffers in Bufferpool: %d\n", runtimeEnv.buffers_in_bufferpool);
    debug_print("  Buffer Size: %d\n", runtimeEnv.buffer_size);
    debug_print("  Tuples per Buffer: %d\n", runtimeEnv.tuples_per_buffer);
    debug_print("  Tuple Size: %d\n", runtimeEnv.tuple_size);
    debug_print("  IFormat: %d\n", runtimeEnv.iformat);
    debug_print("  Sleep Time (ms): %ld\n", runtimeEnv.sleep_time.count());
    debug_print("  RCV Parallelism: %d\n", runtimeEnv.rcv_parallelism);
    debug_print("  Decomp Parallelism: %d\n", runtimeEnv.decomp_parallelism);
    debug_print("  Write Parallelism: %d\n", runtimeEnv.write_parallelism);
    debug_print("  Mode: %d\n", runtimeEnv.mode);
    debug_print("  Monitor: %s\n", runtimeEnv.monitor.load() ? "true" : "false");
}

int xdbcInitialize(XdbcEnvironmentOptions envOpt){
    debug_print("[%s] Initializing new client connection!\n", __func__ );
    try {
        printEnvironmentOptions(envOpt);

        auto it = connectionsMap.find(envOpt.transfer_id);
        if(it != connectionsMap.end()){
            debug_print("[%s] Connection with id %ld already exists! Aborting!\n", __func__, envOpt.transfer_id);
            return -1;
        }
        debug_print("[%s] Checking EnvironmentsOptions...\n", __func__ );
        if(!envOpt.table || !envOpt.server_host || !envOpt.schema_file_with_path){
            debug_print("[%s] No table, server-host or schema file given! Aborting!\n", __func__);
            return -2;
        }

        auto env = std::make_shared<xdbc::RuntimeEnv>();
        initializeEnv(env, envOpt);
        env->env_name = "PostgresFDW Client";
        env->startTime = std::chrono::steady_clock::now();

        //create schema
        std::vector<xdbc::SchemaAttribute> schema;

        string schemaFile = envOpt.schema_file_with_path;

        schema = createSchemaFromConfig(schemaFile);
        env->schemaJSON = readJsonFileIntoString(schemaFile);
        env->schema = schema;
        env->tuple_size = std::accumulate(env->schema.begin(), env->schema.end(), 0,
                                              [](int acc, const xdbc::SchemaAttribute &attr) {
                                                  return acc + attr.size;
                                              });

        env->tuples_per_buffer = (env->buffer_size * 1024 / env->tuple_size);

        printRuntimeEnv(*env);

        debug_print("Input table: %s with tuple size %d and schema:\n%s",
                    env->table.c_str(), env->tuple_size, formatSchema(env->schema).c_str());

        debug_print("[%s] Creating XClient for transfer...\n", __func__ );
        auto client = std::make_shared<xdbc::XClient>(*env);

        debug_print("[%s] Start receiving on the XClient...\n", __func__ );
        client->startReceiving(env->table);

        debug_print("[%s] Storing connection for further usage...\n", __func__ );
        connectionsMap[env->transfer_id] = client;
        envMap[env->transfer_id] = env;

        return 0;
    } catch (const std::exception& e) {
        // Catch all standard exceptions derived from std::exception
        debug_print("[%s] Standard exception caught: %s\n", __func__, e.what());
    } catch (...) {
        // Catch any other types of exceptions
        debug_print("[%s] Unknown exception caught!\n", __func__);
    }

    return -5;
}

void xdbcClose(long transfer_id){
    try {
        debug_print("[%s] close\n", __func__ );
        auto it = connectionsMap.find(transfer_id);
        auto it2 = envMap.find(transfer_id);
        if (it != connectionsMap.end() && it2 != envMap.end()) {
            it->second->finalize();
            debug_print("[%s] Reference count on xclient pointer in map: %ld\n", __func__ , it->second.use_count() );
            connectionsMap.erase(it);
            envMap.erase(it2);
        }
    } catch (const std::exception& e) {
        // Catch all standard exceptions derived from std::exception
        debug_print("[%s] Standard exception caught: %s\n", __func__, e.what());
    } catch (...) {
        // Catch any other types of exceptions
        debug_print("[%s] Unknown exception caught!\n", __func__);
    }
}