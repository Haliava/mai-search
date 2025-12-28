# lab3_plot.py
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def main():
    # Читаем CSV (только первые 1000-5000 слов, иначе график будет перегружен и тяжел)
    # Но для честного закона Ципфа лучше взять все, но график строить хитро
    df = pd.read_csv("zipf_data.csv")
    
    # Берем топ 10000 для наглядности
    limit = 10000
    if len(df) > limit:
        df = df.iloc[:limit]

    ranks = df["Rank"]
    freqs = df["Frequency"]

    plt.figure(figsize=(10, 6))
    
    # Реальные данные (log-log scale)
    plt.loglog(ranks, freqs, label="Experimental Data", color="blue", linewidth=1)
    
    # Теоретический закон Ципфа (подгоним под первую точку)
    # F = C / rank.  C берем как частоту самого популярного слова.
    C = freqs[0]
    theoretical_zipf = [C / r for r in ranks]
    plt.loglog(ranks, theoretical_zipf, label="Theoretical Zipf (k/x)", linestyle="--", color="red")

    plt.title("Zipf's Law Distribution")
    plt.xlabel("Rank (log)")
    plt.ylabel("Frequency (log)")
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.5)
    
    plt.savefig("zipf_graph.png")
    print("График сохранен в zipf_graph.png")
    plt.show()

if __name__ == "__main__":
    main()