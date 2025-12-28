from pymongo import MongoClient
from bs4 import BeautifulSoup
import re

DB_HOST = "localhost"
DB_PORT = 27017
DB_NAME = "search_engine_lab"
COLLECTION = "documents"
OUTPUT_FILE = "corpus_plain.txt"

def clean_text(html):
    soup = BeautifulSoup(html, 'lxml')
    for script in soup(["script", "style", "header", "footer", "nav", "aside"]):
        script.extract()
    text = soup.get_text(separator=' ')
    text = re.sub(r'\s+', ' ', text)
    return text.strip()

def main():
    client = MongoClient(DB_HOST, DB_PORT)
    db = client[DB_NAME]
    col = db[COLLECTION]
    
    total = col.count_documents({"status": "done"})
    print(f"Exporting {total} docs from MongoDB...")
    
    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        cursor = col.find({"status": "done"}, {"raw_html": 1})
        for i, doc in enumerate(cursor):
            if "raw_html" in doc:
                text = clean_text(doc["raw_html"])
                if text:
                    f.write(text + "\n")
            
            if i % 100 == 0:
                print(f"\nProcessed: {i}/{total}", end="")
    
    print(f"\nsaved in {OUTPUT_FILE}")

if __name__ == "__main__":
    main()