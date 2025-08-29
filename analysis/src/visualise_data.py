import polars as pl
from pathlib import Path
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import matplotlib.dates as mdates

def plot_single_opportunity_percentage_change(path_df: pl.DataFrame, save_path: Path):
    """
    Plots the percentage return for a single arbitrage path.
    """
    if path_df is None or path_df.is_empty():
        return

    # 1. Cast necessary columns to floating-point numbers
    path_df = path_df.with_columns(
        [
            pl.col("unrealisedPnl").cast(pl.Float64),
            pl.col("tradedNotional").cast(pl.Float64)
        ]
    )

    # 2. Calculate the return in percentage and add tick index
    path_df = path_df.with_columns(
        (pl.col("unrealisedPnl") / pl.col("tradedNotional") * 100).alias("Return")
    ).with_row_index("tick_index")
    
    # 3. Create the plot
    plt.figure(figsize=(12, 6))
    plt.plot(path_df["tick_index"], path_df["Return"], linestyle='-') 
    
    plt.title('Percentage Return over Time for a Combined Arbitrage Opportunity')
    plt.xlabel('Tick Index')
    plt.ylabel('Return')
    
    plt.grid(True)
    
    # Improve x-axis readability by using a `MaxNLocator`
    # This automatically selects at most N integer ticks to display.
    ax = plt.gca()
    ax.xaxis.set_major_locator(ticker.MaxNLocator(integer=True, nbins=10)) 

    # If the labels are still too close, you can rotate them
    # ax.tick_params(axis='x', rotation=45)

    # 4. Save the figure
    plt.savefig(save_path)
    print(f"Plot saved to {save_path}")

def plot_exchange_rate_over_time(df: pl.DataFrame, save_path: Path):
    """
    Plots the product of rate1, rate2, and rate3 over time.
    The y-axis is formatted to display raw values.
    """
    if df is None or df.is_empty():
        return

    # 1. Prepare the data
    df = df.with_columns(
        [
            pl.col("rate1").cast(pl.Float64),
            pl.col("rate2").cast(pl.Float64),
            pl.col("rate3").cast(pl.Float64)
        ]
    ).with_columns(
        (pl.col("rate1") * pl.col("rate2") * pl.col("rate3")).alias("exchange_product")
    )
    
    df = df.with_columns(
        pl.from_epoch("tickProcessTime", time_unit="ns").alias("timestamp")
    )

    pandas_df = df.to_pandas()
    
    # 2. Create the plot
    plt.figure(figsize=(12, 6))
    
    plt.plot(pandas_df["timestamp"], pandas_df["exchange_product"], color='black', linewidth=0.5)
    
    # Add a horizontal dashed line at y=1.0 for reference
    plt.axhline(y=1.0, color='red', linestyle='--', linewidth=1.5)

    # 3. Format the axes
    plt.title('Exchange Rate over Time')
    plt.xlabel('Time')
    plt.ylabel('Product of Exchange Rates')
    plt.grid(True)
    
    ax = plt.gca()
    
    # Use ScalarFormatter to disable the offset and display full numbers
    formatter = ticker.ScalarFormatter(useOffset=False)
    ax.yaxis.set_major_formatter(formatter)

    # create month day hour min second date formatter
    date_format = mdates.DateFormatter('%b %d %H:%M:%S')
    ax.xaxis.set_major_formatter(date_format)
    plt.gcf().autofmt_xdate()
    
    # 4. Save the figure
    plt.savefig(save_path)
    print(f"Plot saved to {save_path}")