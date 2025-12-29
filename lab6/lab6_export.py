import csv
from pymongo import MongoClient
from bs4 import BeautifulSoup
import re

DB_HOST = "localhost"
DB_PORT = 27017
DB_NAME = "search_engine_lab"
COLLECTION = "documents"
OUTPUT_FILE = "docs_for_index.csv"

def clean_text(html):
    if not html: return ""
    soup = BeautifulSoup(html, 'lxml')
    for script in soup(["script", "style"]):
        script.extract()
    text = soup.get_text(separator=' ')
    return re.sub(r'\s+', ' ', text).strip()

def main():
    client = MongoClient(DB_HOST, DB_PORT)
    col = client[DB_NAME][COLLECTION]
    
    print("starting...")
    
    with open(OUTPUT_FILE, "w", encoding="utf-8", newline='') as f:
        writer = csv.writer(f, delimiter='\t')
        
        cursor = col.find({"status": "done"})
        count = 0
        for doc in cursor:
            url = doc.get("url", "")
            title = url.split("/")[-1].replace("_", " ")
            
            text = clean_text(doc.get("raw_html", ""))
            
            if text:
                writer.writerow([url, title, text])
                count += 1
                if count % 1000 == 0:
                    print(f"\nExported: {count}", end="")
                    
    print(f"\nExported to {OUTPUT_FILE}")

if __name__ == "__main__":
    main()