import os
import polars as pl
from pathlib import Path

from convert_to_parquet import *
from gather_data import *
from visualise_data import *

# --- Configuration ---
# Get the directory where this script is located
# Assumes this script is in the 'scripts' directory
PROJECT_ROOT = Path(__file__).parent.parent
RESOURCES_DIR = PROJECT_ROOT / 'resources'  # Pointing to the 'results' directory
FILE_NAME = "example_trade_data"

# Define the file paths
JSON_FILE_PATH = RESOURCES_DIR / f'{FILE_NAME}.txt'
PARQUET_FILE_PATH = RESOURCES_DIR / f'{FILE_NAME}.parquet'

def convert_file(input_path: Path, output_path: Path) -> int:
    success = convert_with_polars(input_path, output_path)

    if success != 0:
        print("Conversion failed.")
        exit(1)

def print_num_data_points(lazy_df : pl.LazyFrame):
    num_rows = get_number_of_data_points(lazy_df)
    print(f"Number of data points: {num_rows}")

def print_num_arbitrage_opportunities(lazy_df: pl.LazyFrame):
    num_arbitrage_opportunities = get_number_of_arbitrage_opportunities(lazy_df)
    print(f"Number of arbitrage opportunities: {num_arbitrage_opportunities}")

def print_distinct_num_arbitrage_opportunities(df : pl.DataFrame):
    num_distinct_opportunities = df.select("group_id").max().item()
    print(f"Number of distinct arbitrage opportunities: {num_distinct_opportunities}")

def analyse_nth_arbitrage_opportunity(lazy_df: pl.LazyFrame, n: int):
    nth_path_lazy_df = get_nth_opportunity_path_df(lazy_df, n)
    nth_path_df = nth_path_lazy_df.collect()
    plot_single_opportunity_percentage_change(nth_path_df, RESOURCES_DIR / f"{FILE_NAME}_opportunity_{n}_return.png")

def analyse_exchange_rate_product_over_time_period(df: pl.DataFrame):
    plot_exchange_rate_over_time(df, RESOURCES_DIR / f"{FILE_NAME}_exchange_rate_product_over_time.png")

if __name__ == "__main__":
    # convert_file(JSON_FILE_PATH, PARQUET_FILE_PATH)

    # Correctly read a lazy frame
    lazy_df = pl.scan_parquet(PARQUET_FILE_PATH)

    # print_num_data_points(lazy_df)
    # print_num_arbitrage_opportunities(lazy_df)

    # analyse_nth_arbitrage_opportunity(lazy_df, 0)

    # all_data_df = lazy_df.collect()
    # analyse_exchange_rate_product_over_time_period(all_data_df)

    lazy_grouped_arbitrage_opportunities_df = get_grouped_opportunity_path_df(lazy_df)
    grouped_arbitrage_opportunities_df = lazy_grouped_arbitrage_opportunities_df.collect()

    print_distinct_num_arbitrage_opportunities(grouped_arbitrage_opportunities_df)
