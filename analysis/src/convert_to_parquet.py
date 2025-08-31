import polars as pl

def convert_with_polars(json_path, parquet_path):
    """Converts a large newline-delimited JSON file to Parquet."""

    if not json_path.exists():
        print(f"Error: Source file not found at {json_path}")
        return -1

    print("Starting conversion...")
 
    try:
        schema = pl.Schema({
            "bottleneckLeg" : pl.Utf8,
            "unrealisedPnl": pl.Float64,
            "tradedNotional": pl.Float64,
            "orderBookLevels" : pl.Utf8,
            "tickProcessTime" : pl.Int64,
            "tickReceiveTime" : pl.Int64,
            "rate1": pl.Float64,
            "rate2": pl.Float64,
            "rate3": pl.Float64,
            "isArbitrageOpportunity": pl.Boolean

        })

        pl.scan_ndjson(json_path, schema=schema).sink_parquet(parquet_path)

        print("Conversion complete!")
        print(f"Parquet file saved to: {parquet_path}")
        return 0
    except Exception as e:
        print(f"An error occurred during Polars conversion: {e}")
        print("This could be due to an invalid JSON line in the source file.")
        raise
