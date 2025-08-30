import polars as pl

def convert_with_polars(json_path, parquet_path):
    """Converts a large newline-delimited JSON file to Parquet."""

    if not json_path.exists():
        print(f"Error: Source file not found at {json_path}")
        return -1

    print("Starting conversion...")
    try:
        pl.scan_ndjson(json_path).sink_parquet(parquet_path)
        print("Conversion complete!")
        print(f"Parquet file saved to: {parquet_path}")
        return 0
    except Exception as e:
        print(f"An error occurred during Polars conversion: {e}")
        print("This could be due to an invalid JSON line in the source file.")
        raise
