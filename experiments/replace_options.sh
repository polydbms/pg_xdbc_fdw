#!/bin/bash

# Function to display usage instructions
usage() {
    echo "Usage: $0 -t <table> -s <server_host> -i <transfer_id> -b <buffer_size> -p <buffer_pool_size> -n <net_parallelism> -d <decomp_parallelism> -r <read_parallelism>"
    exit 1
}

# Parse command line arguments
while getopts ":t:s:i:b:p:n:d:r:" opt; do
  case $opt in
    t) table_name="$OPTARG"
    ;;
    s) server_host="$OPTARG"
    ;;
    i) transfer_id="$OPTARG"
    ;;
    b) buffer_size="$OPTARG"
    ;;
    p) buffer_pool_size="$OPTARG"
    ;;
    n) net_parallelism="$OPTARG"
    ;;
    d) decomp_parallelism="$OPTARG"
    ;;
    r) read_parallelism="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&2
        usage
    ;;
    :) echo "Option -$OPTARG requires an argument." >&2
       usage
    ;;
  esac
done

# Check if all required arguments are provided
if [ -z "$table_name" ] || [ -z "$server_host" ] || [ -z "$transfer_id" ] || [ -z "$buffer_size" ] || [ -z "$buffer_pool_size" ] || [ -z "$net_parallelism" ] || [ -z "$decomp_parallelism" ] || [ -z "$read_parallelism" ]; then
    usage
fi

# Output file with the transfer_id in the name
output_file="setup_fdw_${table_name%.sql}_${transfer_id}.sql"
input_file="setup_fdw_${table_name%.sql}.sql"

# Replace the options in the SQL file and create the new file
sed -e "s/table '[^']*'/table '$table_name'/" \
    -e "s/server_host '[^']*'/server_host '$server_host'/" \
    -e "s/transfer_id '[^']*'/transfer_id '$transfer_id'/" \
    -e "s/buffer_size '[^']*'/buffer_size '$buffer_size'/" \
    -e "s/buffer_pool_size '[^']*'/buffer_pool_size '$buffer_pool_size'/" \
    -e "s/net_parallelism '[^']*'/net_parallelism '$net_parallelism'/" \
    -e "s/decomp_parallelism '[^']*'/decomp_parallelism '$decomp_parallelism'/" \
    -e "s/read_parallelism '[^']*'/read_parallelism '$read_parallelism'/" \
    "$input_file" > "/tmp/$output_file"

echo "New SQL file created: $output_file"
