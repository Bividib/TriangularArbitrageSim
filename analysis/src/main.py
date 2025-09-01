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

# Last Updated on 01/09/2025 
# Stores values as fractions rather than percentage for ease of use & calculations
# These are Taker fees 
BINANCE_VIP_LEVELS = {
    "VIP_9": 0.00023,
    "VIP_8": 0.00025,
    "VIP_7": 0.00028,
    "VIP_6": 0.00029,
    "VIP_5": 0.00031,
    "VIP_4": 0.00052,
    "VIP_3": 0.0006,
    "VIP_2": 0.001,
    "VIP_1": 0.001,
    "Regular": 0.001,
    "None" : 0.0,
}

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

def print_distinct_num_arbitrage_opportunities(df : pl.LazyFrame):
    num_distinct_opportunities = df.collect().select("group_id").max().item()
    print(f"Number of distinct arbitrage opportunities: {num_distinct_opportunities}")

def analyse_nth_arbitrage_opportunity(lazy_df: pl.LazyFrame, n: int):
    nth_path_lazy_df = get_nth_opportunity_path_df(lazy_df, n)
    nth_path_df = nth_path_lazy_df.collect()
    plot_single_opportunity_percentage_change(nth_path_df, RESOURCES_DIR / f"{FILE_NAME}_opportunity_{n}_return.png")

def analyse_exchange_rate_product_over_time_period(df: pl.DataFrame):
    plot_exchange_rate_over_time(df, RESOURCES_DIR / f"{FILE_NAME}_exchange_rate_product_over_time.png")

def analyse_return_percentage_frequency_table(df: pl.DataFrame):
    return_bins = [0.0, 0.025, 0.05, 0.075, 0.100, 0.200, 0.300, 0.400, 0.500]
    create_and_save_frequency_table(
        df=df,
        column_name="MaxReturn",
        bins=return_bins,
        title="Panel A: Return",
        save_path=Path(RESOURCES_DIR / f"{FILE_NAME}_return_distribution.png")
    )

def analyse_duration_frequency_table(df: pl.DataFrame):
    duration_bins = [0.0, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0]
    create_and_save_frequency_table(
        df=df,
        column_name="Duration",
        bins=duration_bins,
        title="Panel B: Duration",
        save_path=Path(RESOURCES_DIR / f"{FILE_NAME}_duration_distribution.png")
    )

def analyse_traded_notional_frequency_table(df: pl.DataFrame):
    notional_bins = [i/1000 for i in range(1,1000)]
    create_and_save_frequency_table(
        df=df,
        column_name="MaxTradedNotional",
        bins=notional_bins,
        title="Panel C: Tradable Value",
        save_path=Path(RESOURCES_DIR / f"{FILE_NAME}_traded_notional_distribution.png")
    )

def print_average_return_and_notional(df: pl.DataFrame):
    average_return = df.select(pl.mean("AverageReturn")).item()
    average_notional = df.select(pl.mean("AverageTradedNotional")).item()
    print(f"Average Return: {average_return:.5f}%")
    print(f"Average Traded Notional: {average_notional:.5f}")


def analyse_taker_fees_on_arbitrages(df: pl.DataFrame):
    print(df)


def analyse_user_group_profitability(lazy_df: pl.LazyFrame, group1, group2):

    # Compare between VIP 9 and Regular User
    lazy_summarised_grouped_data_with_group1_df = summarise_arbitrages_by_group(lazy_grouped_arbitrage_opportunities_df, vip_level=group1)

    lazy_summarised_grouped_data_with_group2_df = summarise_arbitrages_by_group(lazy_grouped_arbitrage_opportunities_df, vip_level=group2)

    print(lazy_summarised_grouped_data_with_group1_df.collect())

if __name__ == "__main__":
    # convert_file(JSON_FILE_PATH, PARQUET_FILE_PATH)

    # # Correctly read a lazy frame
    lazy_df = pl.scan_parquet(PARQUET_FILE_PATH)
    # print(pl.read_parquet_schema(PARQUET_FILE_PATH))

    # print_num_data_points(lazy_df)
    # print_num_arbitrage_opportunities(lazy_df)

    # analyse_nth_arbitrage_opportunity(lazy_df, 0)

    # all_data_df = lazy_df.collect()
    # analyse_exchange_rate_product_over_time_period(all_data_df)

    lazy_grouped_arbitrage_opportunities_df = get_grouped_opportunity_path_df(lazy_df)
    # print_distinct_num_arbitrage_opportunities(lazy_grouped_arbitrage_opportunities_df)

    lazy_summarised_grouped_data_no_vip_df = summarise_arbitrages_by_group(lazy_grouped_arbitrage_opportunities_df, vip_level="None")
    profitable_vip_level_summary_df = calculate_profitable_opportunities_by_vip(lazy_summarised_grouped_data_no_vip_df, "MaxReturn",BINANCE_VIP_LEVELS)

    analyse_taker_fees_on_arbitrages(profitable_vip_level_summary_df)

    summarised_grouped_data_df = lazy_summarised_grouped_data_no_vip_df.collect()
    analyse_return_percentage_frequency_table(summarised_grouped_data_df)
    analyse_duration_frequency_table(summarised_grouped_data_df)
    analyse_traded_notional_frequency_table(summarised_grouped_data_df)
    # print_average_return_and_notional(summarised_grouped_data_df)
    print(summarised_grouped_data_df)


    # Compare between VIP 9 and Regular User
    print("regular viper stats")
    analyse_user_group_profitability(lazy_grouped_arbitrage_opportunities_df, "VIP_9", "Regular")
