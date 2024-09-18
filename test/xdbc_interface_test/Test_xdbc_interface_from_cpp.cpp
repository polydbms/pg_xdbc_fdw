//
// Created by joel on 04.09.24.
//

#include "Test_xdbc_interface_from_cpp.h"

using namespace std;
namespace po = boost::program_options;


void handleCMDParams(int ac, char *av[], xdbc::RuntimeEnv &env) {
    // Declare the supported options.
    po::options_description desc("Usage: ./test_client [options]\n\nAllowed options");
    desc.add_options()
            ("help,h", "Produce this help message.")
            ("table,e", po::value<string>()->default_value("test_10000000"),
             "Set table: \nDefault:\n  test_10000000")
            ("server-host,a", po::value<string>()->default_value("xdbcserver"),
             "Set server host address: \nDefault:\n  xdbcserver")
            ("intermediate-format,f", po::value<int>()->default_value(1),
             "Set intermediate-format: \nDefault:\n  1 (row)\nOther:\n  2 (col)")
            ("buffer-size,b", po::value<int>()->default_value(64),
             "Set buffer-size of buffers (in KiB).\nDefault: 64")
            ("bufferpool-size,p", po::value<int>()->default_value(4096),
             "Set bufferpool memory size (in KiB).\nDefault: 4096")
            //("tuple-size,t", po::value<int>()->default_value(48), "Set the tuple size.\nDefault: 48")
            ("sleep-time,s", po::value<int>()->default_value(5), "Set a sleep-time in milli seconds.\nDefault: 5ms")
            ("mode,m", po::value<int>()->default_value(1), "1: Analytics, 2: Storage.\nDefault: 1")
            ("net-parallelism,n", po::value<int>()->default_value(1), "Set the network parallelism grade.\nDefault: 1")
            ("write-parallelism,r", po::value<int>()->default_value(1), "Set the read parallelism grade.\nDefault: 1")
            ("decomp-parallelism,d", po::value<int>()->default_value(1),
             "Set the decompression parallelism grade.\nDefault: 1")
            ("transfer-id,tid", po::value<long>()->default_value(0),
             "Set the transfer id.\nDefault: 0");

    po::positional_options_description p;
    p.add("compression-type", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        exit(0);
    }

    if (vm.count("table")) {
        spdlog::get("XCLIENT")->info("Table: {0}", vm["table"].as<string>());
        env.table = vm["table"].as<string>();
    }
    if (vm.count("intermediate-format")) {
        spdlog::get("XCLIENT")->info("Intermediate format: {0}", vm["intermediate-format"].as<int>());
        env.iformat = vm["intermediate-format"].as<int>();
    }

    if (vm.count("buffer-size")) {
        spdlog::get("XCLIENT")->info("Buffer-size: {0} KiB", vm["buffer-size"].as<int>());
        env.buffer_size = vm["buffer-size"].as<int>();
    }
    if (vm.count("bufferpool-size")) {
        spdlog::get("XCLIENT")->info("Bufferpool size: {0} KiB", vm["bufferpool-size"].as<int>());
        env.buffers_in_bufferpool = vm["bufferpool-size"].as<int>() / vm["buffer-size"].as<int>();
        spdlog::get("XCLIENT")->info("Buffers in Bufferpool: {0}", env.buffers_in_bufferpool);
    }
    /*if (vm.count("tuple-size")) {
        spdlog::get("XCLIENT")->info("Tuple size: {0}", vm["tuple-size"].as<int>());
        env.tuple_size = vm["tuple-size"].as<int>();
    }*/
    if (vm.count("sleep-time")) {
        spdlog::get("XCLIENT")->info("Sleep time: {0} ms", vm["sleep-time"].as<int>());
        env.sleep_time = std::chrono::milliseconds(vm["sleep-time"].as<int>());
    }
    if (vm.count("net-parallelism")) {
        spdlog::get("XCLIENT")->info("Network parallelism: {0}", vm["net-parallelism"].as<int>());
        env.rcv_parallelism = vm["net-parallelism"].as<int>();
    }
    if (vm.count("write-parallelism")) {
        spdlog::get("XCLIENT")->info("Write parallelism: {0}", vm["write-parallelism"].as<int>());
        env.write_parallelism = vm["write-parallelism"].as<int>();
    }
    if (vm.count("decomp-parallelism")) {
        spdlog::get("XCLIENT")->info("Decompression parallelism: {0}", vm["decomp-parallelism"].as<int>());
        env.decomp_parallelism = vm["decomp-parallelism"].as<int>();
    }
    if (vm.count("mode")) {
        spdlog::get("XCLIENT")->info("Mode: {0}", vm["mode"].as<int>());
        env.mode = vm["mode"].as<int>();
    }
    if (vm.count("transfer-id")) {
        spdlog::get("XCLIENT")->info("Transfer id: {0}", vm["transfer-id"].as<long>());
        env.transfer_id = vm["transfer-id"].as<long>();
    }
    if (vm.count("server-host")) {
        spdlog::get("XCLIENT")->info("Server host: {0}", vm["server-host"].as<string>());
        env.server_host = vm["server-host"].as<string>();
    }


    //env.server_host = "xdbcserver";
    env.schemaJSON = "";
    env.server_port = "1234";

    env.tuple_size = 0;
    env.tuples_per_buffer = 0;
    env.monitor.store(false);

}


int analyticsThread(int thr, long &totalcnt) {

    int buffsRead = 0;

    XdbcBuffer buf = xdbcGetBuffer(0, thr);

    while(buf.id >= 0){
        totalcnt += buf.tuplesCount;
        xdbcMarkBufferAsRead(0,buf.id);
        buffsRead++;

        buf = xdbcGetBuffer(0,thr);
    }
    spdlog::get("XCLIENT")->warn("Write thread {0} found invalid buffer with id: {1}, buff_no: {2}",
                                 thr, buf.id, buffsRead);

    return 1;

}


void runAnalytics() {

    long totalcnts[read_parallelism];

    std::thread writeThreads[read_parallelism];

    for (int i = 0; i < read_parallelism; i++) {

        totalcnts[i] = 0L;
        writeThreads[i] = std::thread(&analyticsThread, i, std::ref(totalcnts[i]));
    }


    for (int i = 0; i < read_parallelism; i++) {
        writeThreads[i].join();
    }


    long totalcnt = std::accumulate(totalcnts, totalcnts + read_parallelism, 0L);

    spdlog::get("XCLIENT")->info("totalcnt: {0}", totalcnt);
}



int main(int argc, char *argv[]) {

    auto console = spdlog::stdout_color_mt("XCLIENT");

    XdbcEnvironmentOptions envOptOld = xdbcCreateEnvOpt();
    char table[] = "lineitem_sf10";
    envOptOld.table = new char[strlen(table) +1];
    std::strcpy(envOptOld.table, table);
    char server[] = "pg_xdbc_fdw-xdbc-server-1";
    envOptOld.server_host = new char[strlen(server)+1];
    std::strcpy(envOptOld.server_host, server);
    char schemapath[] = "/pg_xdbc_fdw/ressources/schemas/lineitem_sf10.json";
    envOptOld.schema_file_with_path = new char[strlen(schemapath)+1];
    std::strcpy(envOptOld.schema_file_with_path, schemapath);

    envOptOld.read_parallelism = 4;

    xdbcInitialize(envOptOld);

    read_parallelism = envOptOld.read_parallelism;

    runAnalytics();

    xdbcClose(envOptOld.transfer_id);

    return 0;
}

