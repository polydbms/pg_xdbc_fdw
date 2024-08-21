//
// Created by Joel Ziegler on 09.08.2024
//

#include "xdbc_interface_helper.h"
#include "xdbc_interface.h"

void markXdbcBufferAsRead(int transfer_id, int bufferID){
    auto xclient = connectionsMap[transfer_id];
    xclient->markBufferAsRead(bufferID);
}

XdbcBuffer getXdbcBuffer(int transfer_id, int thr){
    auto xclient = connectionsMap[transfer_id];

    while(xclient->hasNext(thr)){
        debug_print("[%s] Waiting for next buffer...\n", __func__);
        xdbc::buffWithId curBuffWithId = xclient->getBuffer(thr);

        if(curBuffWithId.iformat >= 1) {
            auto dataPtr = reinterpret_cast<const unsigned char*>(curBuffWithId.buff.data());
            XdbcBuffer buff{
                    curBuffWithId.id,
                    curBuffWithId.iformat,
                    curBuffWithId.buff.size(),
                    dataPtr
            };
            return buff;
        } else {
            debug_print("[%s] found invalid buffer! id: %d\n", __func__,
                        curBuffWithId.id );
            break;
        }
    }

    XdbcBuffer buff2{
            -1,
            -1,
            0,
            nullptr
    };
    return buff2;
}

//todo this is only for example purposes on how to read from the buffer for now.
int storageThread(int thr, long transfer_id) {

    auto xclient = connectionsMap[transfer_id];
    auto env = envMap[transfer_id];

    int totalcnt = 0;
    int cnt = 0;
    int buffsRead = 0;


    //TODO: refactor to call only when columnar format (2)
    std::vector<size_t> offsets(env->schema.size());
    size_t baseOffset = 0;
    for (size_t i = 0; i < env->schema.size(); ++i) {
        offsets[i] = baseOffset;
        baseOffset += env->tuples_per_buffer * env->schema[i].size;
    }

    while (xclient->hasNext(thr)) {
        // Get next read buffer

        xdbc::buffWithId curBuffWithId = xclient->getBuffer(thr);

        if (curBuffWithId.id >= 0) {
            if (curBuffWithId.iformat == 1) {

                auto dataPtr = curBuffWithId.buff.data();
                for (size_t i = 0; i < env->tuples_per_buffer; ++i) {
                    size_t offset = 0;

                    // Check the first attribute before proceeding
                    //TODO: fix empty tuples by not writing them on the server side
                    if (env->schema.front().tpe == "INT" && *reinterpret_cast<int *>(dataPtr) < 0) {
                        debug_print("[%s] Empty tuple at buffer: %d, tupleNo: %d\n", __func__,
                                                     curBuffWithId.id, cnt);
                        break;
                    }

                    for (const auto &attr: env->schema) {
                        if (attr.tpe == "INT") {
                            //csvBuffer << *reinterpret_cast<int *>(dataPtr + offset);
                            offset += sizeof(int);
                        } else if (attr.tpe == "DOUBLE") {
                            //csvBuffer << *reinterpret_cast<double *>(dataPtr + offset);
                            offset += sizeof(double);
                        } else if (attr.tpe == "CHAR") {
                            //csvBuffer << *reinterpret_cast<char *>(dataPtr + offset);
                            offset += sizeof(char);
                        } else if (attr.tpe == "STRING") {
                            //csvBuffer << reinterpret_cast<char *>(dataPtr + offset);;
                            offset += attr.size;
                        }

                        //csvBuffer << (&attr != &env->schema.back() ? "," : "\n");
                    }

                    cnt++;
                    dataPtr += offset;
                }

                //csvFile << csvBuffer.str();
                //csvBuffer.str("");
            }
            if (curBuffWithId.iformat == 2) {

                std::vector<void *> pointers(env->schema.size());
                std::vector<int *> intPointers(env->schema.size());
                std::vector<double *> doublePointers(env->schema.size());
                std::vector<char *> charPointers(env->schema.size());
                std::vector<char *> stringPointers(env->schema.size());

                std::byte *dataPtr = curBuffWithId.buff.data();

                // Initialize pointers for the current buffer
                for (size_t j = 0; j < env->schema.size(); ++j) {
                    pointers[j] = static_cast<void *>(dataPtr + offsets[j]);
                    if (env->schema[j].tpe == "INT") {
                        intPointers[j] = reinterpret_cast<int *>(pointers[j]);
                    } else if (env->schema[j].tpe == "DOUBLE") {
                        doublePointers[j] = reinterpret_cast<double *>(pointers[j]);
                    } else if (env->schema[j].tpe == "CHAR") {
                        charPointers[j] = reinterpret_cast<char *>(pointers[j]);
                    } else if (env->schema[j].tpe == "STRING") {
                        stringPointers[j] = reinterpret_cast<char *>(pointers[j]);
                    }
                }

                // Loop over rows
                for (int i = 0; i < env->tuples_per_buffer; ++i) {
                    if (*(intPointers[0] + i) < 0) {
                        //spdlog::get("XCLIENT")->warn("Empty tuple at buffer: {0}, tupleNo: {1}", curBuffWithId.id, i);
                        break;  // Exit the loop if the first element is less than zero
                    }
                    for (size_t j = 0; j < env->schema.size(); ++j) {
                        if (env->schema[j].tpe == "INT") {
                            //csvBuffer << *(intPointers[j] + i);
                        } else if (env->schema[j].tpe == "DOUBLE") {
                            //csvBuffer << *(doublePointers[j] + i);
                        } else if (env->schema[j].tpe == "CHAR") {
                            //csvBuffer << *(charPointers[j] + i);
                        } else if (env->schema[j].tpe == "STRING") {
                            //csvBuffer << stringPointers[j] + i * env->schema[j].size;
                        }
                        //csvBuffer << (j < env->schema.size() - 1 ? "," : "\n");
                    }
                }

                //csvFile << csvBuffer.str();
                //csvBuffer.str("");

            }
            buffsRead++;
            xclient->markBufferAsRead(curBuffWithId.id);
        } else {
            debug_print("[%s] found invalid buffer! id: %d, buff_no: %d\n", __func__,
                        curBuffWithId.id, buffsRead );
            break;
        }
    }

    return buffsRead;
}

std::vector<xdbc::SchemaAttribute> createSchemaFromConfig(const std::string &configFile) {
    debug_print("[%s]", __func__);
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
    debug_print("[%s]", __func__);
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
    debug_print("[%s]", __func__);

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

    env->rcv_time = 0;
    env->decomp_time = 0;
    env->write_time = 0;

    env->rcv_wait_time = 0;
    env->decomp_wait_time = 0;
    env->write_wait_time = 0;
    env->tuple_size = 0;
    env->tuples_per_buffer = 0;
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