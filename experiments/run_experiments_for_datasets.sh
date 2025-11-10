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

# # datasets
# # dataset_tables=("lineitem_sf10") 
# dataset_tables=("lineitem_sf10" "ss13husallm" "inputeventsm" "iotm")
# # schemas
# # schema_prefixes=("xdbc")
# schema_prefixes=("jdbc" "native_fdw" "xdbc")

# All possible datasets
all_datasets=("lineitem_sf10" "ss13husallm" "inputeventsm" "iotm")
# All possible schemas
all_schemas=("jdbc" "native_fdw" "xdbc")

# timeout in seconds
timeout_in_seconds=900

# Output directory
out_dir="res"

# result files
all_runs_times="res/query_times.csv"
mean_run_times="res/averages.csv"
client_container_name="xdbcpostgres"
server_container_name="xdbcserver"

run_count=1

# =====================================================================================================

combination_id=${1:-0} # Default to 0 (run all) if not specified

# Function to get combination by ID
get_combination() {
    local id=$1
    case $id in
        1) echo "jdbc lineitem_sf10" ;;
        2) echo "jdbc ss13husallm" ;;
        3) echo "jdbc inputeventsm" ;;
        4) echo "jdbc iotm" ;;
        5) echo "native_fdw lineitem_sf10" ;;
        6) echo "native_fdw ss13husallm" ;;
        7) echo "native_fdw inputeventsm" ;;
        8) echo "native_fdw iotm" ;;
        9) echo "xdbc lineitem_sf10" ;;
        10) echo "xdbc ss13husallm" ;;
        11) echo "xdbc inputeventsm" ;;
        12) echo "xdbc iotm" ;;
        *) echo "" ;;
    esac
}

if [ $combination_id -eq 0 ]; then
    # Run all combinations
    schema_prefixes=("${all_schemas[@]}")
    dataset_tables=("${all_datasets[@]}")
    echo "Running all 12 combinations with $run_count runs each"
else
    # Run specific combination
    combination=$(get_combination $combination_id)
    if [ -z "$combination" ]; then
        echo "Invalid combination_id: $combination_id. Must be 1-12."
        exit 1
    fi
    
    read -r schema table <<< "$combination"
    schema_prefixes=("$schema")
    dataset_tables=("$table")
    echo "Running combination $combination_id: $schema + $table with $run_count runs"
fi

# rm -rf "${mean_run_times}"

# # Make a new output directory for a new experiment run
# if [ -d "${out_dir}" ]; then
#     echo "Removing existing directory: ${out_dir}"
#     rm -rf "${out_dir}"
# fi
# sleep 5

# mkdir -p "${out_dir}"

# Result CSV files
csv_file="${all_runs_times}"
av_file="${mean_run_times}"

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

        # Check if we should skip execution (jdbc + lineitem_sf10)
    if [ "$fdw" = "jdbc" ] && [ "$dataset_name" = "lineitem_sf10" ]; then
        
        for ((i = 1; i <= runcount; i++)); do
            echo "$fdw,$dataset_name,$i,0" >> "$csv"
        done
        
        echo "$fdw,$dataset_name,0" >> "$averages_file"
        return
    fi

    for ((i = 1; i <= runcount; i++)); do
            echo "run ${i}/${runcount}..."
            if [ $TIMEOUT_FLAG -eq 1 ]; then
                    # query already timed out in an earlier iteration, so just skip it
                    echo "$fdw,$dataset_name,$i,$timeout" >> "$csv"
                    sumtime=$(awk -v s="$sumtime" -v r="$timeout" 'BEGIN{printf "%.9f\n", s + r}')
                    continue
            fi

            if [ "$fdw" = "xdbc" ]; then
                docker exec ${server_container_name} pkill -f xdbc-server 2>/dev/null || true
                sleep 1
                #docker exec -d -w /xdbc-server/build/ "${server_container_name}" ./xdbc-server -y postgres -b 1024 -p 32768 --deser-parallelism=4 --read-parallelism=8
                docker exec -d ${server_container_name} bash -c "./xdbc-server/build/xdbc-server --system postgres -b 1024 -p 32768 --deser-parallelism 4 --read-parallelism 8"
                sleep 3
                echo "Started xdbc server for this fdw."
            else
                echo "Running baseline. Not starting xdbc server for this fdw since it is not required."
            fi

            # Measure the execution time in psql
            start_time=$(date +%s.%N)  # Start time in seconds since epoch
            docker exec "${client_container_name}" \
              psql db1 -v ON_ERROR_STOP=1 -c "SET statement_timeout = ${TIMEOUT_DURATION}; COPY (select * from ${table}) TO '/tmp/out' WITH CSV HEADER;" > /dev/null 2>&1
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
    docker exec ${server_container_name} pkill -f xdbc-server 2>/dev/null || true

    # Output average of the runs of the query
    average=$(awk -v s="$sumtime" -v n="$runcount" 'BEGIN{printf "%.9f\n", s / n}')
    echo "$fdw,$dataset_name,$average" >> "$averages_file"
}

total_iterations=$(( ${#schema_prefixes[@]} * ${#dataset_tables[@]} ))
current_iteration=0

for schema in "${schema_prefixes[@]}"; do
  for tablename in "${dataset_tables[@]}"; do
    current_iteration=$((current_iteration + 1))
    echo "=== Iteration ${current_iteration}/${total_iterations}: ${schema}.${tablename} ==="
    run_queries "${schema}.${tablename}" "$csv_file" "${schema}" "$run_count" "$av_file" $timeout_in_seconds "${tablename}"
  done
done

echo "Query execution times recorded in $csv_file"
echo "Average time for every query in $av_file"


