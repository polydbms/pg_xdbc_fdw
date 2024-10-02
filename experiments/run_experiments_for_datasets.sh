#!/bin/bash
set -v

# USAGE
#
# 1. Set dataset table names and schema prefixes
#     for each schema prefix this script does "select *" on the schema.tablename and stores the time
#     if schema is xdbc, the xdbc server is started beforehand.
# 2. Set general variables like output directory, file names, container names and timeout
#
#
#
#


# ==================================================  VARIABLES ======================================

# datasets
dataset_tables=("pg1_sf0001_lineitem") #"pg1_sf1_lineitem" "pg1_sf10_lineitem")

# schemas
schema_prefixes=("xdbc" "native_fdw" "jdbc")

# timeout in seconds
timeout_in_seconds=3600

# Output directory
out_dir="experiment_results"

# result files
all_runs_times="query_times.csv"
mean_run_times="averages.csv"
client_container_name="pg_xdbc_client"
server_container_name="pg_xdbc_server"

# =====================================================================================================



# Make a new output directory for a new experiment run

# Initialize the directory counter
counter=1

# Loop to find the next available directory name
while [ -d "${out_dir}${counter}" ]; do
    counter=$((counter + 1))
done

# Create the next incremented directory
mkdir "${out_dir}${counter}"

# Output the name of the created directory
echo "Created directory: ${out_dir}${counter}"

out_dir="${out_dir}${counter}/"

# Result CSV files
csv_file="${out_dir}${all_runs_times}"
av_file="${out_dir}${mean_run_times}"

# Create or clear the CSV file
echo "fdw,dataset,run,execution_time" > "$csv_file"
echo "fdw,dataset,execution_time" > "$av_file"

# Function to run queries and record execution time
run_queries() {
    local table="$1"  # table name that should be select * queried
    local csv="$2"  # CSV file to record results
    local fdw="$3" # used fdw, which is used as entry "fdw" in the result csv
    local runcount=$4  # how many times each query should be executed
    local averages_file="$5"  # csv file for averages
    local timeout=$6
    local dataset_name=$7
    local TIMEOUT_DURATION
    TIMEOUT_DURATION=$(awk -v s="$timeout" 'BEGIN{printf "%d\n", s*1000}')


    # run each query multiple times

    TIMEOUT_FLAG=0
    sumtime=0

    for ((i = 1; i <= runcount; i++)); do
            echo "run ${i}/${runcount}..."
            if [ $TIMEOUT_FLAG -eq 1 ]; then
                    # query already timed out in an earlier iteration, so just skip it
                    echo "$fdw,$dataset_name,$i,$timeout" >> "$csv"
                    sumtime=$(awk -v s="$sumtime" -v r="$timeout" 'BEGIN{printf "%.9f\n", s + r}')
                    continue
            fi

            if [ "$fdw" = "xdbc" ]; then
                docker exec -d -w /xdbc-server/build/ "${server_container_name}" ./xdbc-server -y postgres -b 1024 -p 32768 --deser-parallelism=4 --read-parallelism=8
                sleep 1
                echo "Started xdbc server for this fdw."
            else
                echo "Not starting xdbc server for this fdw."
            fi

            # Measure the execution time in psql
            start_time=$(date +%s.%N)  # Start time in seconds since epoch
            docker exec "${client_container_name}" \
              psql db1 -v ON_ERROR_STOP=1 -c "SET statement_timeout = ${TIMEOUT_DURATION};" "select * from ${table};" > /dev/null 2>&1
            end_time=$(date +%s.%N)  # End time in seconds since epoch

            # Calculate execution time
            execution_time=$(awk "BEGIN {print $end_time - $start_time}")

            if (( $(echo "$execution_time > $timeout" | bc -l) )); then
                    # query timed out. Just skipp the other executions
                    echo "----- query timed out!"
                    echo "$fdw,$dataset_name,$i,$timeout" >> "$csv"
                    TIMEOUT_FLAG=1
            else
                    # Output filename and execution time to the CSV file
                    echo "$fdw,$dataset_name,$i,$execution_time" >> "$csv"
            fi

            sumtime=$(awk -v s="$sumtime" -v r="$execution_time" 'BEGIN{printf "%.9f\n", s + r}')

            if [ "$fdw" = "xdbc" ]; then
                sleep 5
            fi
    done

    # Output average of the runs of the query
    average=$(awk -v s="$sumtime" -v n="$runcount" 'BEGIN{printf "%.9f\n", s / n}')
    echo "$fdw,$dataset_name,$average" >> "$averages_file"
}

for schema in "${schema_prefixes[@]}"; do
  for tablename in "${dataset_tables[@]}"; do
    run_queries "${schema}.${tablename}" "$csv_file" "${schema}" "$1" "$av_file" $timeout_in_seconds "${tablename}"
  done
done

echo "Query execution times recorded in $csv_file"
echo "Average time for every query in $av_file"


