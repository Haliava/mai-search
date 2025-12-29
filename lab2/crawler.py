import time
import yaml
import requests
from pymongo import MongoClient
from urllib.parse import urljoin, urlparse, urldefrag
from bs4 import BeautifulSoup
from datetime import datetime, timezone

class Crawler:
    def __init__(self, config_path):
        with open(config_path, 'r') as file:
            self.config = yaml.safe_load(file)

        self.client = MongoClient(self.config['db']['host'], self.config['db']['port'])
        self.db = self.client[self.config['db']['name']]
        self.collection = self.db[self.config['db']['collection']]
        
        self.collection.create_index("url", unique=True)
        self.collection.create_index("status")

        self.session = requests.Session()
        self.session.headers.update({'User-Agent': self.config['logic']['user_agent']})
        
        self.allowed_domains = [
            "en.wikipedia.org",
            "aviation.fandom.com"
        ]

    def normalize_url(self, base, link):
        if not link:
            return None
        full_url = urljoin(base, link)
        full_url, _ = urldefrag(full_url)
        parsed = urlparse(full_url)
        
        if parsed.scheme not in ['http', 'https']:
            return None
        
        domain_ok = any(domain in parsed.netloc for domain in self.allowed_domains)
        if not domain_ok:
            return None
            
        if "action=" in parsed.query or "/Special:" in parsed.path or "/File:" in parsed.path or "/Talk:" in parsed.path:
            return None

        return full_url

    def add_seeds(self):
        for url in self.config['seeds']:
            try:
                self.collection.insert_one({
                    "url": url,
                    "status": "new",  # new, processing, done, error
                    "source": "seed",
                    "attempts": 0
                })
                print(f"Seed added: {url}")
            except Exception:
                pass

    def get_next_url(self):
        return self.collection.find_one_and_update(
            {"status": "new"},
            {"$set": {"status": "processing", "start_time": datetime.now(timezone.utc)}}
        )

    def extract_links(self, html, base_url):
        soup = BeautifulSoup(html, 'lxml')
        links = set()
        for a_tag in soup.find_all('a', href=True):
            normalized = self.normalize_url(base_url, a_tag['href'])
            if normalized:
                links.add(normalized)
        return links

    def determine_source_name(self, url):
        if "wikipedia.org" in url:
            return "Wikipedia"
        elif "fandom.com" in url:
            return "Fandom"
        return "Unknown"

    def run(self):
        self.add_seeds()
        
        count = self.collection.count_documents({"status": "done"})
        max_docs = self.config['logic']['max_docs']

        print(f"Starting crawler. Goal: {max_docs} documents")
        
        while count < max_docs:
            doc = self.get_next_url()
            if not doc:
                print("Queue is empty")
                break
            
            url = doc['url']
            print(f"[{count+1}/{max_docs}] Crawling: {url}")

            try:
                time.sleep(self.config['logic']['delay'])
                
                response = self.session.get(url, timeout=10)
                
                if response.status_code == 200:
                    html = response.text
                    source_name = self.determine_source_name(url)
                    
                    self.collection.update_one(
                        {"_id": doc["_id"]},
                        {"$set": {
                            "status": "done",
                            "raw_html": html,
                            "source": source_name,
                            "crawled_at": datetime.now(timezone.utc).timestamp(),
                            "http_code": 200
                        }}
                    )
                    
                    if self.collection.estimated_document_count() < max_docs * 2:
                        new_links = self.extract_links(html, url)
                        for link in new_links:
                            try:
                                self.collection.insert_one({
                                    "url": link,
                                    "status": "new",
                                    "attempts": 0
                                })
                            except Exception:
                                pass
                    
                    count += 1
                else:
                    print(f"Error {response.status_code} for {url}")
                    self.collection.update_one(
                        {"_id": doc["_id"]},
                        {"$set": {"status": "error", "http_code": response.status_code}}
                    )

            except Exception as e:
                print(f"Exception for {url}: {e}")
                self.collection.update_one(
                    {"_id": doc["_id"]},
                    {"$set": {"status": "error", "error_msg": str(e)}}
                )

        print("Crawling finished")

if __name__ == "__main__":
    bot = Crawler("config.yaml")
    bot.run()