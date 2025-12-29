#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <chrono>

const std::string INPUT_FILE = "docs_for_index.csv";
const std::string FILE_DOCS_DATA = "docs_data.bin";
const std::string FILE_DOCS_INDEX = "docs_index.bin";
const std::string FILE_TERM_INDEX = "term_index.bin";
const std::string FILE_POSTINGS = "postings.bin";

struct RawEntry {
    std::string term;
    uint32_t doc_id;

    bool operator<(const RawEntry& other) const {
        if (term != other.term)
            return term < other.term;
        return doc_id < other.doc_id;
    }
};

bool is_separator(char c) {
    if ((unsigned char)c > 127) return false;
    return !isalnum(c);
}

char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

int main() {
    std::cout << "starting..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ifstream fin(INPUT_FILE);
    if (!fin.is_open()) {
        std::cerr << "err" << std::endl;
        return 1;
    }

    std::ofstream f_docs_data(FILE_DOCS_DATA, std::ios::binary);
    std::ofstream f_docs_index(FILE_DOCS_INDEX, std::ios::binary);

    std::vector<RawEntry> entries; 
    
    std::string line;
    uint32_t doc_id = 0;
    
    uint64_t total_bytes_indexed = 0;

    std::cout << "1. Parsing and index construction..." << std::endl;

    while (std::getline(fin, line)) {
        std::stringstream ss(line);
        std::string url, title, text;
        
        std::getline(ss, url, '\t');
        std::getline(ss, title, '\t');
        std::getline(ss, text, '\t');

        if (text.empty()) continue;

        uint64_t offset = f_docs_data.tellp();
        f_docs_index.write(reinterpret_cast<const char*>(&offset), sizeof(offset));

        uint16_t url_len = url.size();
        f_docs_data.write(reinterpret_cast<const char*>(&url_len), sizeof(url_len));
        f_docs_data.write(url.c_str(), url_len);

        uint16_t title_len = title.size();
        f_docs_data.write(reinterpret_cast<const char*>(&title_len), sizeof(title_len));
        f_docs_data.write(title.c_str(), title_len);

        std::string token;
        total_bytes_indexed += text.size();
        
        for (char c : text) {
            if (is_separator(c)) {
                if (!token.empty()) {
                    entries.push_back({token, doc_id});
                    token.clear();
                }
            } else {
                token += to_lower(c);
            }
        }
        if (!token.empty()) {
            entries.push_back({token, doc_id});
        }

        doc_id++;
        if (doc_id % 1000 == 0) std::cout << "\rProcessed docs: " << doc_id << std::flush;
    }
    
    f_docs_data.close();
    f_docs_index.close();
    
    std::cout << "\n2. Sorting " << entries.size() << " entries..." << std::endl;
    std::sort(entries.begin(), entries.end());

    std::cout << "3. Writing inverted index..." << std::endl;
    
    std::ofstream f_term_index(FILE_TERM_INDEX, std::ios::binary);
    std::ofstream f_postings(FILE_POSTINGS, std::ios::binary);

    if (entries.empty()) return 0;

    std::string current_term = entries[0].term;
    std::vector<uint32_t> current_postings;
    current_postings.push_back(entries[0].doc_id);
    
    uint64_t unique_terms = 0;

    for (size_t i = 1; i < entries.size(); ++i) {
        if (entries[i].term == current_term) {
            if (entries[i].doc_id != current_postings.back()) {
                current_postings.push_back(entries[i].doc_id);
            }
        } else {
            uint64_t postings_offset = f_postings.tellp();
            uint32_t count = current_postings.size();
            
            for (uint32_t id : current_postings) {
                f_postings.write(reinterpret_cast<const char*>(&id), sizeof(id));
            }

            uint16_t term_len = current_term.size();
            f_term_index.write(reinterpret_cast<const char*>(&term_len), sizeof(term_len));
            f_term_index.write(current_term.c_str(), term_len);
            f_term_index.write(reinterpret_cast<const char*>(&postings_offset), sizeof(postings_offset));
            f_term_index.write(reinterpret_cast<const char*>(&count), sizeof(count));

            unique_terms++;

            current_term = entries[i].term;
            current_postings.clear();
            current_postings.push_back(entries[i].doc_id);
        }
    }
    
    uint64_t postings_offset = f_postings.tellp();
    uint32_t count = current_postings.size();
    for (uint32_t id : current_postings) {
        f_postings.write(reinterpret_cast<const char*>(&id), sizeof(id));
    }
    uint16_t term_len = current_term.size();
    f_term_index.write(reinterpret_cast<const char*>(&term_len), sizeof(term_len));
    f_term_index.write(current_term.c_str(), term_len);
    f_term_index.write(reinterpret_cast<const char*>(&postings_offset), sizeof(postings_offset));
    f_term_index.write(reinterpret_cast<const char*>(&count), sizeof(count));
    unique_terms++;

    f_term_index.close();
    f_postings.close();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "\n--- Индексация завершена ---" << std::endl;
    std::cout << "Документов: " << doc_id << std::endl;
    std::cout << "Всего терм: " << entries.size() << std::endl;
    std::cout << "Уникальных терм: " << unique_terms << std::endl;
    std::cout << "Время: " << elapsed.count() << " с" << std::endl;
    std::cout << "Скорость индексации: " << (doc_id / elapsed.count()) << " доков/с" << std::endl;
    std::cout << "КБ/с: " << (total_bytes_indexed / 1024.0 / elapsed.count()) << std::endl;

    return 0;
}