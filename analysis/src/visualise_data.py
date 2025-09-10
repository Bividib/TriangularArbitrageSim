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
    ax.ticklabel_format(style="plain", axis="y", useOffset=False)  # Disable scientific notation on y-axis

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

def create_and_save_frequency_table(
    df: pl.DataFrame,
    column_name: str,
    bins: List[float],
    title: str,
    save_path: Path
):
    """
    Creates and saves a frequency distribution table as an image.
    """
    if df is None or df.is_empty():
        print("DataFrame is empty, skipping table creation.")
        return

    # 1. Get the frequency data using your existing helper function
    freq_df = _create_frequency_table_df(df, column_name, bins)

    # 2. Prepare data for rendering
    # Select and format the columns to match the example table
    table_df = freq_df.select([
        "bin_label",
        pl.col("Frequency").cast(str),
        pl.col("Rel. frequency").round(2).cast(str),
        pl.col("Total").cast(str),
        pl.col("Rel. total").round(2).cast(str)
    ])
    
    col_headers = ["Frequency", "Rel. frequency", "Total", "Rel. total"]
    row_headers = table_df["bin_label"].to_list()
    cell_text = table_df.drop("bin_label").to_numpy()

    # 3. Create the table image
    fig, ax = plt.subplots(figsize=(8, 4)) # Adjust figsize as needed
    ax.axis('tight')
    ax.axis('off') # Hide the axes

    # Create the table object
    table = ax.table(
        cellText=cell_text,
        colLabels=col_headers,
        rowLabels=row_headers,
        loc='center',
        cellLoc='right',
        colLoc='center'
    )
    
    # Style the table to look professional
    table.auto_set_font_size(False)
    table.set_fontsize(10)
    table.scale(1.2, 1.5) # Adjust cell padding

    # 4. Add the title above the table
    plt.title(title, y=1.08) # Adjust y to position the title

    # 5. Save the figure
    # bbox_inches='tight' is crucial for removing excess whitespace
    plt.savefig(save_path, bbox_inches='tight', pad_inches=0.1)
    print(f"Table for '{column_name}' saved to {save_path}")
    plt.close(fig) # Close the figure to free up memory



def create_simple_table(df: pl.DataFrame, title: str, save_path: str):
    """
    Creates a clean visual table from a Polars DataFrame and saves it as an image.
    """
    # 1. Round all floating point numbers for cleaner display
    df_clean = df.with_columns(
        pl.col(pl.Float64).round(6)
    )

    # Add a row count column, starting from 1
    df_with_count = df_clean.with_row_index(name="#", offset=1)
    df_with_count = df_with_count.select(pl.col("#"), pl.all().exclude("#"))

    # Prepare data for matplotlib
    column_headers = df_with_count.columns
    cell_text = df_with_count.rows()

    # 2. Make the figure size dynamic based on the data size
    num_rows, num_cols = df_with_count.shape
    # Adjust the multipliers as needed for your preference
    fig_width = num_cols * 2.2 
    fig_height = (num_rows + 1) * 0.4
    fig, ax = plt.subplots(figsize=(fig_width, fig_height))
    
    ax.axis('tight')
    ax.axis('off')

    # Create the table
    table = ax.table(
        cellText=cell_text,
        colLabels=column_headers,
        cellLoc='center',
        loc='center'
    )
    table.set_fontsize(12)
    table.scale(1, 1.5) # Adjust height scaling

    # 3. Automatically adjust column widths
    table.auto_set_column_width(col=list(range(num_cols)))
    
    # Set title
    plt.title(title, fontsize=16, pad=20)
    
    # Save the figure
    Path(save_path).parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(save_path, dpi=300, bbox_inches='tight', pad_inches=0.2)
    plt.close()
    print(f"Table saved to {save_path}")


def _filter_bin_and_count(
    df: pl.DataFrame, 
    bin_on_col: str, 
    filter_on_col: str, 
    bins: list, 
    labels: list
) -> pl.DataFrame:
    """
    Filters a DataFrame based on a condition on `filter_on_col` (>=0),
    then bins and counts the frequency of values in `bin_on_col`.
    """
    return (
        df.filter(pl.col(filter_on_col).is_not_null())
        .filter(pl.col(bin_on_col).is_not_null())
        .filter(pl.col(filter_on_col) >= 0)  # Condition applied to filter_on_col
        .with_columns(
            # Binning is performed on bin_on_col
            pl.col(bin_on_col).cut(bins, labels=labels, include_breaks=True).alias("binned_data")
        )
        .unnest("binned_data")
        .group_by("category", "breakpoint")
        # The aggregated column is named after the binned column
        .agg(pl.len().alias(bin_on_col))
        .sort("breakpoint")
        .rename({"category": "bin"})
        .select("bin", bin_on_col)
    )


def create_profitability_comparison_table(
    df1: pl.DataFrame,
    df2: pl.DataFrame,
    group1_name: str,
    group2_name: str,
    bins: list,
    column_map: dict[str, str],  # Changed from a list to a dictionary
    col_headers_display: List[str],
    save_path: Path
):

    labels = [f"< {bins[0]:.3f}"]
    labels += [f"{bins[i]:.3f}-{bins[i+1]:.3f}" for i in range(len(bins) - 1)]
    labels.append(f">{bins[-1]:.3f}")

    final_df = pl.DataFrame({"bin": labels}).with_columns(pl.col("bin").cast(pl.Categorical))
    
    # --- Data processing using the new helper and column_map ---
    for bin_col, filter_col in column_map.items():
        binned_g1 = _filter_bin_and_count(df1, bin_col, filter_col, bins, labels)
        final_df = final_df.join(binned_g1, on="bin", how="left")
        
    for bin_col, filter_col in column_map.items():
        binned_g2 = _filter_bin_and_count(df2, bin_col, filter_col, bins, labels)
        final_df = final_df.join(binned_g2, on="bin", how="left")
    
    final_df = final_df.slice(1, len(final_df) - 2)
    
    numeric_df = final_df.drop("bin").fill_null(0)
    totals_row = numeric_df.sum()
    cell_text_body = numeric_df.to_numpy()
    cell_text_total = totals_row.to_numpy()
    cell_text = np.vstack([cell_text_body, cell_text_total])
    row_labels = final_df["bin"].to_list()
    row_labels.append("Σ Total")

    full_col_headers = col_headers_display * 2

    # --- Plotting ---
    fig, ax = plt.subplots(figsize=(10, 5.5))
    ax.axis('off')
    
    table = ax.table(
        cellText=cell_text,
        rowLabels=row_labels,
        colLabels=full_col_headers,
        loc='center',
        cellLoc='center'
    )
    table.auto_set_font_size(False)
    table.set_fontsize(10)
    table.scale(1, 1.8)
    table.auto_set_column_width(col=list(range(len(full_col_headers))))

    # --- Styling last row---
    # The table object's row index starts at 1 for data rows because of the header.
    # So, the last row is at index len(row_labels).
    total_row_index = len(row_labels)
    for i in range(len(full_col_headers)):
        table[total_row_index, i].get_text().set_weight('bold')
    table.get_celld()[(total_row_index, -1)].get_text().set_weight('bold')

    ax.text(0.45, 1.001, group1_name, transform=ax.transAxes, ha='center', va='center', weight='bold', fontsize=12)
    ax.text(0.55, 1.001, group2_name, transform=ax.transAxes, ha='center', va='center', weight='bold', fontsize=12)

    
    plt.savefig(save_path, dpi=300, bbox_inches='tight', pad_inches=0.1)
    plt.close(fig)
    print(f"User comparison table saved to {save_path}")