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
# FILE_NAME = "BtcUsdtEthBtc-20250825-210237"
FILE_NAME = "example_trade_data"

# Define the file paths
JSON_FILE_PATH = RESOURCES_DIR / f'{FILE_NAME}.txt'
PARQUET_FILE_PATH = RESOURCES_DIR / f'{FILE_NAME}.parquet'

# Last Updated on 01/09/2025 
# Stores values as fractions rather than percentage for ease of use & calculations
# These are Taker fees 
BINANCE_VIP_LEVELS = {
    "None" : 0.0,
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
}

def convert_file(input_path: Path, output_path: Path) -> int:
    success = convert_with_polars(input_path, output_path)

    if success != 0:
        print("Conversion failed.")
        exit(1)

def analyse_individual_data_points(lazy_df: pl.LazyFrame):
    result_df = summarise_all_data_points(lazy_df)
    create_simple_table(result_df, "Summary of All Data Points", RESOURCES_DIR / f"{FILE_NAME}_all_data_points_summary.png")

def analyse_nth_arbitrage_opportunity(
    lazy_df: pl.LazyFrame, 
    n: int, 
    duration_s: float = 0.0, 
    comparison: str = 'gt',
    min_rows: int = 1
):
    # Pass the new parameter to the data gathering function
    nth_path_lazy_df = get_nth_opportunity_path_df(lazy_df, n, duration_s, comparison, min_rows)
    nth_path_df = nth_path_lazy_df.collect()
    
    # --- Dynamic Filename ---
    # The comparison variable is now used directly in the filename
    plot_single_opportunity_percentage_change(
        nth_path_df, 
        RESOURCES_DIR / f"{FILE_NAME}_opportunity_{n}{f'_{comparison}_{duration_s}s' if duration_s > 0 else ''}_{min_rows}_rows_return.png"
    )

def analyse_exchange_rate_product_over_time_period(df: pl.DataFrame):
    plot_exchange_rate_over_time(df, RESOURCES_DIR / f"{FILE_NAME}_exchange_rate_product_over_time.png")

def analyse_return_percentage_frequency_table(df: pl.DataFrame):
    return_bins = [0.01, 0.025, 0.05, 0.075, 0.1, 0.15, 0.2]
    create_and_save_frequency_table(
        df=df,
        column_name="MaxReturn",
        bins=return_bins,
        title="Panel A: Return",
        save_path=Path(RESOURCES_DIR / f"{FILE_NAME}_return_distribution.png")
    )

def analyse_duration_frequency_table(df: pl.DataFrame):
    duration_bins = [0.5, 1.0, 2.0, 5.0, 10.0, 30.0, 200.0]
    create_and_save_frequency_table(
        df=df,
        column_name="Duration",
        bins=duration_bins,
        title="Panel B: Duration",
        save_path=Path(RESOURCES_DIR / f"{FILE_NAME}_duration_distribution.png")
    )

def analyse_traded_notional_frequency_table(df: pl.DataFrame):
    notional_bins = [0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 8.0]
    create_and_save_frequency_table(
        df=df,
        column_name="MaxTradedNotional",
        bins=notional_bins,
        title="Panel C: Tradable Value",
        save_path=Path(RESOURCES_DIR / f"{FILE_NAME}_traded_notional_distribution.png")
    )

def analyse_all_distinct_arbitrages_summary(df: pl.DataFrame):
    create_simple_table(df, "Summary Statistics of All Distinct Arbitrage Opportunities", RESOURCES_DIR / f"{FILE_NAME}_all_distinct_arbitrages_summary.png")

def analyse_taker_fees_on_arbitrages(df: pl.LazyFrame):
    profitable_vip_level_summary_df = calculate_profitable_opportunities_by_vip(df, "MaxReturn",BINANCE_VIP_LEVELS)
    create_simple_table(profitable_vip_level_summary_df, "Profitable Opportunities by VIP Level", RESOURCES_DIR / f"{FILE_NAME}_profitable_opportunities_by_vip_level.png")


def analyse_user_group_profitability(lazy_grouped_arbitrage_opportunities_df: pl.LazyFrame, group1: str, group2: str):
    """
    Gets summary data for two user groups and calls the plotting function.
    """
    print(f"\n--- Analysing User Group Profitability: {group1} vs {group2} ---")
    
    # 1. Get the summarized data for both groups (no pre-filtering)
    df1 = summarise_arbitrages_by_group(lazy_grouped_arbitrage_opportunities_df, vip_level=group1).collect()
    df2 = summarise_arbitrages_by_group(lazy_grouped_arbitrage_opportunities_df, vip_level=group2).collect()

    # 2. Define the bins for the frequency table
    return_bins = [0.0, 0.01, 0.025, 0.05, 0.075, 0.1, 0.15, 0.2]

    return_columns = ["FirstReturn", "MaxReturn", "ReturnForMaxTradedNotional", "AverageReturn"]
    col_headers_display = ["First", "Max", "Highest\nvalue", "Average"]
    
    # 3. Call the plotting function with the full, un-filtered summary data
    create_profitability_comparison_table(
        df1=df1,
        df2=df2,
        group1_name=group1,
        group2_name=group2,
        return_bins=return_bins,
        return_columns=return_columns,
        col_headers_display=col_headers_display,
        save_path=RESOURCES_DIR / f"{FILE_NAME}_profitability_comparison_{group1}_vs_{group2}.png"
    )


def analyse_bottleneck_leg_distribution(df : pl.DataFrame):
    distribution_df = get_bottleneck_leg_distribution(df)
    create_simple_table(distribution_df, "Bottleneck Leg Distribution", RESOURCES_DIR / f"{FILE_NAME}_bottleneck_leg_distribution.png")

if __name__ == "__main__":
    # convert_file(JSON_FILE_PATH, PARQUET_FILE_PATH)

    # # Correctly read a lazy frame
    lazy_df = pl.scan_parquet(PARQUET_FILE_PATH)
    # print(pl.read_parquet_schema(PARQUET_FILE_PATH))

    analyse_individual_data_points(lazy_df)
    # analyse_nth_arbitrage_opportunity(lazy_df, 0, 2,'lt',10)

    # all_data_df = lazy_df.collect()
    # analyse_exchange_rate_product_over_time_period(all_data_df)
    # analyse_bottleneck_leg_distribution(all_data_df)

    # lazy_grouped_arbitrage_opportunities_df = get_grouped_opportunity_path_df(lazy_df)

    # lazy_summarised_grouped_data_no_vip_df = summarise_arbitrages_by_group(lazy_grouped_arbitrage_opportunities_df, vip_level="None")

    # analyse_taker_fees_on_arbitrages(lazy_summarised_grouped_data_no_vip_df)

    # summarised_grouped_data_df = lazy_summarised_grouped_data_no_vip_df.collect()
    # print(summarised_grouped_data_df)

    # analyse_return_percentage_frequency_table(summarised_grouped_data_df)
    # analyse_duration_frequency_table(summarised_grouped_data_df)
    # analyse_traded_notional_frequency_table(summarised_grouped_data_df)

    # summarised_all_arbitrages_df = summarise_all_arbitrages(lazy_summarised_grouped_data_no_vip_df)
    # analyse_all_distinct_arbitrages_summary(summarised_all_arbitrages_df)

    # # Compare between VIP 9 and VIP 5
    # analyse_user_group_profitability(lazy_grouped_arbitrage_opportunities_df, "VIP_9", "VIP_5")
