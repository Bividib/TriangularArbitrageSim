#!/bin/bash

# ==============================================================================
# Generic Docker Arbitrage Bot Runner
#
# This script is a generic template to run a Dockerized arbitrage bot.
# It takes all configuration parameters as command-line arguments.
#
# Usage:
#   ./run_arbitrage_bot.sh <CONTAINER_NAME> <OUTPUT_FILE_BASE> <ARBITRAGE_PATH> <STREAM_TARGET>
#
# Before running:
#   You must have Docker installed.
#   Set your DOCKER_USERNAME and DOCKER_TOKEN as environment variables.
#   e.g., export DOCKER_USERNAME="your-username"
#         export DOCKER_TOKEN="your-token"
#
# Optional environment variables for the C++ application (overridden if passed):
#   - BINANCE_PROFIT_THRESHOLD
#   - BINANCE_TAKER_FEE
#   - BINANCE_MAX_STARTING_NOTIONAL_FRACTION
# ==============================================================================

# --- Argument Parsing ---
if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <CONTAINER_NAME> <OUTPUT_FILE_BASE> <ARBITRAGE_PATH> <STREAM_TARGET>"
    exit 1
fi

CONTAINER_NAME="$1"
OUTPUT_FILE_BASE="$2"
BINANCE_ARBITRAGE_PATH="$3"
BINANCE_STREAM_TARGET="$4"

# --- Static Configuration ---
IMAGE_NAME="bividib/triangular-arbitrage-sim"
CONTAINER_DATA_DIR="/app/data"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

if [ -z "$DOCKER_USERNAME" ] || [ -z "$DOCKER_TOKEN" ]; then
    echo "Error: DOCKER_USERNAME and DOCKER_TOKEN environment variables must be set."
    exit 1
fi

DATETIME=$(date +"%Y%m%d-%H%M%S")
FILENAME="${OUTPUT_FILE_BASE}-${DATETIME}.txt"
echo "--- Generating dynamic output filename: $FILENAME ---"

HOST_SAVE_PATH="$PROJECT_ROOT/results/${FILENAME}"
CONTAINER_WRITE_PATH="$CONTAINER_DATA_DIR/${FILENAME}"

mkdir -p "$PROJECT_ROOT/results"
touch "$HOST_SAVE_PATH"

echo "--- Host output directory created: $HOST_SAVE_PATH ---"

# Corrected syntax for default values
BINANCE_PROFIT_THRESHOLD="${BINANCE_PROFIT_THRESHOLD:-0}"
BINANCE_TAKER_FEE="${BINANCE_TAKER_FEE:-0}"
BINANCE_MAX_STARTING_NOTIONAL_FRACTION="${BINANCE_MAX_STARTING_NOTIONAL_FRACTION:-0.8}"

# --- Script Execution ---
set -e

echo "--- Logging in to Docker Hub as user '$DOCKER_USERNAME'... ---"
echo "$DOCKER_TOKEN" | docker login -u "$DOCKER_USERNAME" --password-stdin

echo ""
echo "--- Successfully logged in. Pulling image: $IMAGE_NAME ---"
docker pull "$IMAGE_NAME"

echo ""
echo "--- Image pull complete. Stopping and removing existing container '$CONTAINER_NAME'... ---"
docker stop "$CONTAINER_NAME" >/dev/null 2>&1 || true
docker rm "$CONTAINER_NAME" >/dev/null 2>&1 || true

DOCKER_COMMAND="docker run \
    --name \"$CONTAINER_NAME\" -d \
    -e \"BINANCE_UPDATE_FILE_PATH=$CONTAINER_WRITE_PATH\" \
    -e \"BINANCE_STREAM_TARGET=$BINANCE_STREAM_TARGET\" \
    -e \"BINANCE_ARBITRAGE_PATH=$BINANCE_ARBITRAGE_PATH\" \
    -e \"BINANCE_PROFIT_THRESHOLD=$BINANCE_PROFIT_THRESHOLD\" \
    -e \"BINANCE_TAKER_FEE=$BINANCE_TAKER_FEE\" \
    -e \"BINANCE_MAX_STARTING_NOTIONAL_FRACTION=$BINANCE_MAX_STARTING_NOTIONAL_FRACTION\" \
    -v \"$HOST_SAVE_PATH:$CONTAINER_WRITE_PATH\" \
    \"$IMAGE_NAME\""

echo "Executing: $DOCKER_COMMAND"
eval $DOCKER_COMMAND

echo ""
echo "--- Container is running. Use 'docker ps' to verify. ---"
echo "--- Output will be saved to '$HOST_SAVE_PATH' on your host machine. ---"

docker logout

echo ""
echo "--- Script finished successfully. ---"