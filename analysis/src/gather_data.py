import polars as pl

from main import BINANCE_VIP_LEVELS

def summarise_all_data_points(df: pl.LazyFrame) -> pl.DataFrame:
    return df.select(
        pl.len().alias("TotalDataPoints"),
        pl.col("isArbitrageOpportunity").sum().alias("TotalArbitrageOpportunities"),
        (pl.col("isArbitrageOpportunity").sum() / pl.len() * 100).alias("% Opportunities"),
        ((pl.col("tickProcessTime") - pl.col("tickReceiveTime")) / 1_000_000_000).mean().alias("AverageProcessingDelay (s)"),
    ).collect()

def get_nth_opportunity_path_df(
    lazy_df: pl.LazyFrame,
    n: int,
    duration_s: float = 0.0,
    comparison: str = 'gt',
    min_rows: int = 1
) -> pl.LazyFrame:
    """
    Finds the nth group of consecutive arbitrage opportunities, returning all data points in that sequence.
    Can filter by duration and minimum number of ticks.
    """
    opportunity_groups_df = get_grouped_opportunity_path_df(lazy_df)
    duration_ns = int(duration_s * 1_000_000_000)

    # Define the filter conditions
    duration_filter = (
        pl.col("Duration") >= duration_ns
        if comparison == 'gt' else
        pl.col("Duration") <= duration_ns
    )
    # The duration filter is only applied if duration_s > 0
    if duration_s <= 0:
        duration_filter = pl.lit(True) # Always true, effectively disabling the filter

    # Find all group_ids that meet the criteria
    eligible_opportunities_info_df = (
        opportunity_groups_df
        .group_by("group_id")
        .agg(
            (pl.max("tickReceiveTime") - pl.min("tickReceiveTime")).alias("Duration"),
            pl.len().alias("RowCount")
        )
        .filter(duration_filter & (pl.col("RowCount") >= min_rows))
        .select(pl.col("group_id").sort())
        .collect()
    )

    if eligible_opportunities_info_df.is_empty():
        print(f"No arbitrage opportunities found with at least {min_rows} rows.")
        return pl.LazyFrame({})

    if n >= eligible_opportunities_info_df.height:
        print(f"Error: Requested opportunity {n}, but only {eligible_opportunities_info_df.height} matching opportunities exist.")
        return pl.LazyFrame({})

    nth_group_id = eligible_opportunities_info_df.item(n, "group_id")
    return opportunity_groups_df.filter(pl.col("group_id") == nth_group_id)

def get_grouped_opportunity_path_df(lazy_df: pl.LazyFrame) -> pl.LazyFrame:
    """
    Groups the DataFrame by consecutive arbitrage opportunities,
    returning a LazyFrame with an additional 'group_id' column.
    """
    return lazy_df.with_columns(
        (pl.col("isArbitrageOpportunity") & ~pl.col("isArbitrageOpportunity").shift(1).fill_null(False))
        .cast(pl.Int8).cum_sum().alias("group_id")
    ).filter(pl.col("isArbitrageOpportunity"))


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

        # 5. Duration of the opportunity in seconds (from nanoseconds)
        ((pl.max("tickReceiveTime") - pl.min("tickReceiveTime")) / 1_000_000_000).alias("Duration")
    )

def summarise_all_arbitrages(df: pl.LazyFrame) -> pl.DataFrame:
    """
    Summarizes statistics across all distinct arbitrage opportunities. 
    
    Args:
        df (pl.LazyFrame): The DataFrame containing grouped data, resulting from summarise_arbitrages_by_group.
    """

    return df.select(
        pl.len().alias("NumDistinctOpportunities"),
        pl.mean("MaxReturn").alias("AverageMaxReturn"),
        pl.mean("MaxTradedNotional").alias("AverageMaxTradedNotional"),
        
        (pl.mean("Duration")).alias("AverageDuration (s)"),
        
        pl.max("MaxReturn").alias("MaxReturn"),
        pl.max("MaxTradedNotional").alias("MaxTradedNotional"),
        
        (pl.max("Duration")).alias("MaxDuration (s)")
    ).collect()

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

    for level, fee in vip_levels.items():

        # Filter for opportunities where the return exceeds the transaction cost
        profitable_count = (
            df
            .filter(_is_vip_trade_profitable(pl.col(return_col_name), level))
            .select(pl.count())
            .collect()
            .item() 
        )
        
        results_data.append({"VIP Level": level, "Fee" : fee, "ProfitableOpportunities": profitable_count})

    # Create the final summary DataFrame
    summary_table = pl.DataFrame(results_data)
    
    return summary_table


def get_bottleneck_leg_distribution(df : pl.DataFrame) -> pl.DataFrame:
    """
    Calculates the percentage distribution of values in the 'bottleneckLeg' column.

    It assumes the input DataFrame contains a column named 'bottleneckLeg' with
    exactly three distinct values representing the different legs of a trade.

    Args:
        df: A Polars DataFrame containing the trading data.

    Returns:
        A new Polars DataFrame with columns 'bottleneckLeg' and 'percentage',
        showing the distribution of each leg, sorted by leg name.
    """

    # Get the total number of opportunities to use as the denominator
    total_count = df.height

    # Use value_counts() to get the frequency of each leg
    leg_counts = df.get_column("bottleneckLeg").value_counts()

    # Calculate the percentage distribution and rename the columns for clarity
    distribution_df = leg_counts.with_columns(
        ((pl.col("count") / total_count) * 100).alias("percentage")
    ).select(
        pl.col("bottleneckLeg"),
        pl.col("percentage")
    ).sort("bottleneckLeg")
    
    return distribution_df

def _is_vip_trade_profitable(return_percentage, vip_level):
    fee = BINANCE_VIP_LEVELS[vip_level]
    fee_multiplier = (1 - fee) ** 3

    return return_percentage / 100 >= ((1 - fee_multiplier) / fee_multiplier)

def _adjust_rate_for_vip(traded_notional, unrealised_pnl, vip_level):
    fee = BINANCE_VIP_LEVELS[vip_level]
    fee_multiplier = (1 - fee) ** 3

    return (((traded_notional + unrealised_pnl) * fee_multiplier - traded_notional) / traded_notional) * 100


