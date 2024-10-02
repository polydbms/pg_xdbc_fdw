#!/bin/bash

# This script executes sql files of subfolders in docker containers
# multiple times, notes the time of each execute and
# calculates the average of all executions. It outputs the results into a new subfolder with
# two csv files, one for all taken times and one for averages.

# Usage:
# Fill in the variables for output directory, times csv and averages csv file.
# Also set the name for the docker container in which the files are executed (client_container_name).
# The server name is for starting xdbc in the server container before each execute. It
# only gets called, if the xdbc fdw is used.
#
# At the end of this file you call the run_queries() function for each different type of fdw you want to use.
#
# Then call ./run_eqperiments repeats
# where repeats are the number of times each query should be repeated and the average time calculated.
#
# The parameter are as follows:
#
# run_queries "folder_name" "$csv_file" "fdw_name" "$1" "$av_file" timeout
#   folder_name is the name of the subfolder with the sql queries, that should be executed an marked with the
#   fdw_name in the file
#   $csv_file, which has the execution times as rows.
#   $av_file holds the averages of all the runs of the same query
#   timeout in seconds after which executions of a query are aborted and all runs are given the timeout time.

set -v

# Output directory
out_dir="experiment_results"

# result files
all_runs_times="query_times.csv"
mean_run_times="averages.csv"
client_container_name="pg_xdbc_client"
server_container_name="pg_xdbc_server"

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
    local dir="$1"  # Directory containing the query files
    local csv="$2"  # CSV file to record results
    local fdw="$3" # used fdw
    local runcount=$4  # how many times each query should be executed
    local averages_file="$5"  # csv file for averages
    local timeout=$6
    local TIMEOUT_DURATION
    TIMEOUT_DURATION=$(awk -v s="$timeout" 'BEGIN{printf "%d\n", s*1000}')

    # Loop through each file in the specified directory
    for file in "$dir"/*.sql; do
        echo "executing ${file}..."

        # run each query multiple times

        TIMEOUT_FLAG=0

        sumtime=0
        for ((i = 1; i <= runcount; i++)); do
                echo "run ${i}/${runcount}..."
                if [ $TIMEOUT_FLAG -eq 1 ]; then
                        # query already timed out in an earlier iteration, so just skip it
                        echo "$fdw,$(basename -s ".sql" "$file"),$i,$timeout" >> "$csv"
                        sumtime=$(awk -v s="$sumtime" -v r="$timeout" 'BEGIN{printf "%.9f\n", s + r}')
                        continue
                fi

                if [ "$fdw" = "xdbc" ]; then
                    docker exec -d -w /xdbc-server/build/ "${server_container_name}" ./xdbc-server -y postgres -b 1024 -p 32768 --deser-parallelism=4 --read-parallelism=8
                    sleep 1
                else
                    echo "Not starting xdbc server for this fdw."
                fi

                # Measure the execution time in psql
                start_time=$(date +%s.%N)  # Start time in seconds since epoch
                docker exec "${client_container_name}" \
                  psql db1 -v ON_ERROR_STOP=1 -c "SET statement_timeout = ${TIMEOUT_DURATION};" -f "/pg_xdbc_fdw/experiments/${file}" > /dev/null 2>&1
                end_time=$(date +%s.%N)  # End time in seconds since epoch

                # Calculate execution time
                execution_time=$(awk "BEGIN {print $end_time - $start_time}")

                if (( $(echo "$execution_time > $timeout" | bc -l) )); then
                        # query timed out. Just skipp the other executions
                        echo "----- query timed out!"
                        echo "$fdw,$(basename -s ".sql" "$file"),$i,$timeout" >> "$csv"
                        TIMEOUT_FLAG=1
                else
                        # Output filename and execution time to the CSV file
                        echo "$fdw,$(basename -s ".sql" "$file"),$i,$execution_time" >> "$csv"
                fi

                sumtime=$(awk -v s="$sumtime" -v r="$execution_time" 'BEGIN{printf "%.9f\n", s + r}')

                if [ "$fdw" = "xdbc" ]; then
                    sleep 5
                fi
        done

        # Output average of the runs of the query
        average=$(awk -v s="$sumtime" -v n="$runcount" 'BEGIN{printf "%.9f\n", s / n}')
        echo "$fdw,$(basename -s ".sql" "$file"),$average" >> "$averages_file"
    done
}


# Run queries via xdbc
run_queries "queries_xdbc" "$csv_file" "xdbc" "$1" "$av_file" 7200

# Run queries via native postgresql fdw
run_queries "queries_native" "$csv_file" "native" "$1" "$av_file" 7200

# Run queries via jdbc fdw
run_queries "queries_jdbc" "$csv_file" "jdbc" "$1" "$av_file" 7200

echo "Query execution times recorded in $csv_file"
echo "Average time for every query in $av_file"


