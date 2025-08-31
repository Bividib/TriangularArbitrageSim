import polars as pl
from pathlib import Path
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import matplotlib.dates as mdates
import numpy as np
from typing import List, Tuple

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

    first_tick_time = path_df.select(pl.first("tickReceiveTime")).item()
    last_tick_time = path_df.select(pl.last("tickReceiveTime")).item()
    duration_seconds = (last_tick_time - first_tick_time) / 1_000_000_000
    
    # 3. Create the plot
    plt.figure(figsize=(12, 6))
    plt.plot(path_df["tick_index"], path_df["Return"], linestyle='-') 
    title_text = (f'Percentage Return over Time for a {duration_seconds:.2f} second Arbitrage Opportunity')
    plt.title(title_text)
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

def _create_frequency_table_df(
    df: pl.DataFrame,
    column_name: str,
    bins: List[float]
) -> pl.DataFrame:
    """
    Creates a frequency table using cut(include_breaks=True)
    with relative and cumulative percentages.
    The first label is hardcoded to start from 0.
    """
    # 1. Generate N+1 labels with a hardcoded first label starting from 0
    # The first label is now hardcoded as '0 - first_bin_value'
    labels = [f"0.000-{bins[0]:.3f}"]
    # The middle labels 
    labels += [f"{bins[i]:.3f}-{bins[i+1]:.3f}" for i in range(len(bins) - 1)]
    # The final "greater than" label based on last bin value
    labels.append(f">{bins[-1]:.3f}")

    # 2. Bin the data (this part remains unchanged)
    binned_df = df.with_columns(
        pl.col(column_name).cut(
            bins,
            labels=labels,
            include_breaks=True,
        ).alias("binned_data")
    ).unnest("binned_data")

    # 3. Group, sort, and calculate frequency (remains unchanged)
    frequency_df = (
        binned_df.group_by("category", "breakpoint")
        .agg(pl.len().alias("Frequency"))
        .sort("breakpoint")
    )

    # 4. Calculate total (remains unchanged)
    total_count = frequency_df["Frequency"].sum()

    # 5. Calculate percentages and rename column (remains unchanged)
    frequency_df = frequency_df.with_columns(
        (pl.col("Frequency") / total_count * 100).alias("Rel. frequency"),
        (pl.col("Frequency").cum_sum()).alias("Total")
    ).with_columns(
        (pl.col("Total") / total_count * 100).alias("Rel. total")
    ).rename({"category": "bin_label"})

    return frequency_df

def create_and_save_frequency_plot(
    df: pl.DataFrame,
    column_name: str,
    bins: List[float],
    title: str,
    save_path: Path
):
    """
    Creates and saves a bar chart with a cumulative frequency line.

    Args:
        df: The input Polars DataFrame.
        column_name: The column to plot.
        bins: The list of bin edges.
        title: The title of the plot.
        save_path: The path to save the generated image.
    """
    if df is None or df.is_empty():
        print("DataFrame is empty, skipping plot.")
        return

    # Create the frequency table using the helper function
    freq_df = _create_frequency_table_df(df, column_name, bins)

    # Convert the bin labels to a list for plotting
    x = freq_df["bin_label"].to_list()
    y_freq = freq_df["Rel. frequency"].to_list()
    y_cumul = freq_df["Rel. total"].to_list()

    # Create the plot with a secondary y-axis for the cumulative line
    fig, ax1 = plt.subplots(figsize=(12, 6))

    # Bar chart on the primary axis
    ax1.bar(x, y_freq, color='skyblue', label='Relative Frequency')
    ax1.set_xlabel(column_name.capitalize())
    ax1.set_ylabel('Relative Frequency (%)', color='skyblue')
    ax1.tick_params(axis='y', labelcolor='skyblue')
    ax1.tick_params(axis='x', rotation=45)
    ax1.grid(axis='y', linestyle='--', alpha=0.7)

    # Line chart on the secondary axis
    ax2 = ax1.twinx()
    ax2.plot(x, y_cumul, color='red', marker='o', linestyle='-', label='Relative Cumulative Frequency')
    ax2.set_ylabel('Relative Cumulative Frequency (%)', color='red')
    ax2.tick_params(axis='y', labelcolor='red')

    # Add a title and legend
    plt.title(title)
    
    # Combine legends from both axes
    lines, labels = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax2.legend(lines + lines2, labels + labels2, loc='upper left')

    # Ensure the plot is saved without cutting off labels
    plt.tight_layout()
    plt.savefig(save_path)
    print(f"Plot for '{column_name}' saved to {save_path}")