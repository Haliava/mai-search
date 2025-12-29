# lab3_plot.py
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def main():
    df = pd.read_csv("zipf_data.csv")
    
    limit = 10000
    if len(df) > limit:
        df = df.iloc[:limit]

    ranks = df["Rank"]
    freqs = df["Frequency"]

    plt.figure(figsize=(10, 6))
    
    plt.loglog(ranks, freqs, label="Experimental Data", color="blue", linewidth=1)
    
    C = freqs[0]
    theoretical_zipf = [C / r for r in ranks]
    plt.loglog(ranks, theoretical_zipf, label="Theoretical Zipf (k/x)", linestyle="--", color="red")

    plt.title("Zipf's Law")
    plt.xlabel("Rank (log)")
    plt.ylabel("Frequency (log)")
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.5)
    
    plt.savefig("zipf_graph.png")
    print("saved to zipf_graph.png")
    plt.show()

if __name__ == "__main__":
    main()