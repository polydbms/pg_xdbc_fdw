//
// Created by Joel Ziegler on 09.08.2024
//

#include "xdbc_interface_helper.h"
#include "xdbc_interface.h"

void markXdbcBufferAsRead(int transfer_id, int bufferID){
    auto xclient = connectionsMap[transfer_id];
    xclient->markBufferAsRead(bufferID);
}

//todo maybe take an uninitialized XdbcBuffer * as an argument and fill it and return error codes as int.
XdbcBuffer getXdbcBuffer(int transfer_id, int thr){
    try {
        debug_print("[%s]\n", __func__);
        auto it = connectionsMap.find(transfer_id);
        if (it != connectionsMap.end()) {
            auto xclient = it->second;
            while (xclient->hasNext(thr)) {
                debug_print("[%s] Waiting for next buffer...\n", __func__);
                xdbc::buffWithId curBuffWithId = xclient->getBuffer(thr);
                debug_print("[%s] Got next buffer...\n", __func__);


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
            debug_print("[%s] Couldn't find xclient for transfer_id: %d\n", __func__, transfer_id );
        }
    } catch (const std::exception& e) {
        // Catch all standard exceptions derived from std::exception
        std::cerr << "Standard exception caught: " << e.what() << std::endl;
    } catch (...) {
        // Catch any other types of exceptions
        std::cerr << "Unknown exception caught" << std::endl;
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

std::vector<xdbc::SchemaAttribute> createSchemaFromConfig(const std::string &configFile) {
    debug_print("[%s]\n", __func__);
    std::ifstream file(configFile);
    if (!file.is_open()) {
        debug_print("Failed to open schema: %s\n", configFile.c_str());
    }
    nlohmann::json schemaJson;
    file >> schemaJson;

    std::vector<xdbc::SchemaAttribute> schema;
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
    debug_print("[%s]\n", __func__);
    std::ifstream file(filePath);
    if (!file.is_open()) {
        debug_print("Failed to open schema: %s", filePath.c_str());
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

EnvironmentOptions createEnvOpt(){
    EnvironmentOptions env;
    env.intermediate_format = 1;
    env.buffer_size = 64;
    env.bufferpool_size = 4096;
    env.sleep_time = 5;
    env.mode = 1;
    env.net_parallelism = 1;
    env.read_parallelism = 1;
    env.decomp_parallelism = 4;
    env.transfer_id = 0;
    env.table = nullptr;
    env.server_host = nullptr;
    env.schema_file_with_path = nullptr;
    env.tuple_size = 0;
    return env;
}

void initializeEnv(const std::shared_ptr<xdbc::RuntimeEnv>& env, EnvironmentOptions &envOpt){
    debug_print("[%s]\n", __func__);

    // assign transaction parameters
    env->table = envOpt.table;
    env->server_host = envOpt.server_host;
    env->iformat = envOpt.intermediate_format;
    env->buffer_size = envOpt.buffer_size;
    env->buffers_in_bufferpool = envOpt.buffer_size / envOpt.bufferpool_size;
    env->tuple_size = envOpt.tuple_size;
    env->sleep_time = std::chrono::milliseconds(envOpt.sleep_time);
    env->rcv_parallelism = envOpt.net_parallelism;
    env->write_parallelism = envOpt.read_parallelism;
    env->decomp_parallelism = envOpt.decomp_parallelism;
    env->mode = envOpt.mode;
    env->transfer_id = envOpt.transfer_id;

    // assign static values
    env->schemaJSON = "";
    env->server_port = "1234";

    env->tuple_size = 0;
    env->tuples_per_buffer = 0;
    env->monitor.store(false);
}


int xdbcInitialize(EnvironmentOptions envOpt){
    debug_print("[%s]", __func__);

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

    debug_print("[%s] Creating RuntimeEnv object...\n", __func__ );
    auto env = std::make_shared<xdbc::RuntimeEnv>();
    initializeEnv(env, envOpt);
    env->env_name = "PostgresFDW Client";

    debug_print("[%s] Loading schema file into RuntimeEnv...\n", __func__ );
    //create schema
    std::vector<xdbc::SchemaAttribute> schema;

    std::string schemaFile = envOpt.schema_file_with_path;

    schema = createSchemaFromConfig(schemaFile);
    env->schemaJSON = readJsonFileIntoString(schemaFile);
    env->schema = schema;
    env->tuple_size = std::accumulate(env->schema.begin(), env->schema.end(), 0,
                                     [](int acc, const xdbc::SchemaAttribute &attr) {
                                         return acc + attr.size;
                                     });

    env->tuples_per_buffer = env->buffer_size * 1024 / env->tuple_size;

    debug_print("[%s] Creating XClient for transfer...\n", __func__ );
    auto client = std::make_shared<xdbc::XClient>(*env);
    client->startReceiving(env->table);

    debug_print("[%s] Storing connection for further usage...\n", __func__ );
    connectionsMap[env->transfer_id] = client;
    envMap[env->transfer_id] = env;

    return 0;
}

void close(){
    //xclient.finalize();
}