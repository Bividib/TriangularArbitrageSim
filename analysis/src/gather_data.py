import polars as pl

from main import BINANCE_VIP_LEVELS

def get_number_of_data_points(df: pl.LazyFrame) -> int:
    return df.select(pl.len()).collect().item()

# count the number of arbitrage opportunities
def get_number_of_arbitrage_opportunities(df: pl.LazyFrame) -> int:
    # Implement your logic to count arbitrage opportunities
    return df.filter(pl.col("isArbitrageOpportunity") == True).select(pl.len()).collect().item()

def get_nth_opportunity_path_df(lazy_df: pl.LazyFrame, n: int) -> pl.LazyFrame:
    """
    Finds the nth group of consecutive arbitrage opportunities,
    returning all data points in that sequence.
    """
    # 1. Identify consecutive groups of TRUE values.
    df_with_groups = lazy_df.with_columns(
        (pl.col("isArbitrageOpportunity") & ~pl.col("isArbitrageOpportunity").shift(1).fill_null(False))
        .cast(pl.Int8).cum_sum().alias("group_id")
    )

    # 2. Collect information about each opportunity group.
    # We filter for only the arbitrage opportunities to find the groups.
    nth_opportunity_info_df = df_with_groups.filter(pl.col("isArbitrageOpportunity")).select(
        pl.col("group_id").unique().sort().alias("group_id")
    ).collect()

    if nth_opportunity_info_df.is_empty():
        print("No arbitrage opportunities found.")
        return pl.LazyFrame({})
    
    # Check if the requested opportunity 'n' exists
    if n >= nth_opportunity_info_df.height:
        print(f"Opportunity {n} not found. Only {nth_opportunity_info_df.height} opportunities exist.")
        return pl.LazyFrame({})

    # 3. Extract group_id for the requested nth group.
    nth_group_id = nth_opportunity_info_df.item(n, "group_id")

    # 4. Filter the original data for the correct group.
    # Ensure we take only 
    nth_group_df = df_with_groups.filter(
        (pl.col("group_id") == nth_group_id) & (pl.col("isArbitrageOpportunity") == True)
    )

    return nth_group_df

def get_grouped_opportunity_path_df(lazy_df: pl.LazyFrame) -> pl.LazyFrame:
    """
    Groups the DataFrame by consecutive arbitrage opportunities,
    returning a LazyFrame with an additional 'group_id' column.
    """
    # 1. Identify consecutive groups of TRUE values.
    df_with_groups = lazy_df.with_columns(
        (pl.col("isArbitrageOpportunity") & ~pl.col("isArbitrageOpportunity").shift(1).fill_null(False))
        .cast(pl.Int8).cum_sum().alias("group_id")
    )

    # 2. Cast the columns to Float64 to prevent InvalidOperationError
    df_with_correct_types = lazy_df.with_columns(
        pl.col("unrealisedPnl").cast(pl.Float64),
        pl.col("tradedNotional").cast(pl.Float64)
    )

    # 3. Filter to keep only rows where isArbitrageOpportunity is TRUE
    grouped_opportunities_df = df_with_groups.filter(pl.col("isArbitrageOpportunity"))

    return grouped_opportunities_df

def summarise_arbitrages_by_group(grouped_df: pl.LazyFrame, vip_level: str) -> pl.LazyFrame:
    """
    Summarizes each arbitrage opportunity group (defining exact moment of execution)

    1) The first tick (trader assume exceeding theshold of 1 and return is above transaction costs)
    2) The best profit (trader realises when the best profit will be - forseeing the future)
    3) Returns from the highest tradable value (trader identifies the point of highest liquidity)
    4) Average return and average tradable value
    5) Duration
    """

    # Define the indexes for faster searching 
    max_notional_idx = pl.col("tradedNotional").arg_max()
    return_expr = _adjust_rate_for_vip(pl.col("tradedNotional"), pl.col("unrealisedPnl"), vip_level)
    max_return_idx = return_expr.arg_max()

    return grouped_df.group_by("group_id").agg(
        # 1. First
        (_adjust_rate_for_vip(pl.first("tradedNotional"), pl.first("unrealisedPnl"), vip_level)).alias("FirstReturn"),
        (pl.first("tradedNotional")).alias("FirstTradedNotional"),

        # 2. Best profit
        return_expr.max().alias("MaxReturn"),
        pl.col("tradedNotional").get(max_return_idx).alias("TradedNotionalForMaxReturn"),
        
        # 3. Highest value
        (_adjust_rate_for_vip(pl.col("tradedNotional").max(), pl.col("unrealisedPnl").get(max_notional_idx), vip_level)).alias("ReturnForMaxTradedNotional"),
        (pl.max("tradedNotional")).alias("MaxTradedNotional"),
        
        # 4. Average
        (_adjust_rate_for_vip(pl.col("tradedNotional"),pl.col("unrealisedPnl"),vip_level)).mean().alias("AverageReturn"),
        (pl.mean("tradedNotional")).alias("AverageTradedNotional"),

        # 5. Duration of the opportunity
        (pl.max("tickReceiveTime") - pl.min("tickReceiveTime")).alias("Duration")
    )

def calculate_profitable_opportunities_by_vip(df: pl.LazyFrame, return_col_name: str, vip_levels: dict) -> pl.DataFrame:
    """
    Calculate profitable opportunities for a specific VIP trader.

    Args:
        row_df (pl.LazyFrame): The DataFrame containing trading data.
        return_col_name (str): The name of the return column to analyze, assumed to be % return
        vip_levels (dict): A dictionary mapping VIP levels to their transaction fees.

    Returns:
        pl.LazyFrame: A DataFrame with profitable opportunities for the specified VIP trader.
    """
    results_data = []

    for level, _ in vip_levels.items():

        # Filter for opportunities where the return exceeds the transaction cost
        profitable_count = (
            df
            .filter(_is_vip_trade_profitable(pl.col(return_col_name), level))
            .select(pl.count())
            .collect()
            .item() 
        )
        
        results_data.append({"VIP Level": level, "ProfitableOpportunities": profitable_count})

    # Create the final summary DataFrame
    summary_table = pl.DataFrame(results_data)
    
    return summary_table

def _is_vip_trade_profitable(return_percentage, vip_level):
    fee = BINANCE_VIP_LEVELS[vip_level]
    fee_multiplier = (1 - fee) ** 3

    return return_percentage / 100 > ((1 - fee_multiplier) / fee_multiplier)

def _adjust_rate_for_vip(traded_notional, unrealised_pnl, vip_level):
    fee = BINANCE_VIP_LEVELS[vip_level]
    fee_multiplier = (1 - fee) ** 3

    return (((traded_notional + unrealised_pnl) * fee_multiplier - traded_notional) / traded_notional) * 100