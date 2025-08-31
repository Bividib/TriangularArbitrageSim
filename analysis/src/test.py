# test_polars.py
import polars as pl

# 1. Print the version of Polars to see what we're working with
print(f"Polars version: {pl.__version__}")

# 2. Recreate the exact scenario that is failing in your code
df = pl.DataFrame({
    "Return": [0.01, 0.03, 0.06, 0.15, 0.45, 0.8]
})

bins = [0.01, 0.02, 0.03, 0.04, 0.05]
labels = ["a","b","c","d","e","f"]

print(f"\nNumber of breaks: {len(bins)}")
print(f"Number of labels: {len(labels)}")

df = pl.DataFrame({"foo": [0, 1, 2, 3,4]})

result = df.with_columns(
   pl.col("foo").cut([1,3], include_breaks=True, labels=["0-1", "1-3","3+"]).alias("cut")
).unnest("cut")

print(result)