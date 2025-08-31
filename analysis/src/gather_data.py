import polars as pl

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

def summarise_arbitrages_by_group(grouped_df: pl.LazyFrame) -> pl.LazyFrame:
    """
    Summarizes each arbitrage opportunity group, calculating the return percentage and traded notional
    based on the first element of each group as well as the time duration of the whole opportunity.
    """

    return grouped_df.group_by("group_id").agg(
        (pl.first("unrealisedPnl") / pl.first("tradedNotional") * 100).alias("Return"),
        (pl.last("tickReceiveTime") - pl.first("tickReceiveTime")).alias("Duration"),
        (pl.first("tradedNotional")).alias("TradedNotional")
    )