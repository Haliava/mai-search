#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <chrono>

const std::string FILE_DOCS_DATA = "../lab6/docs_data.bin";
const std::string FILE_DOCS_INDEX = "../lab6/docs_index.bin";
const std::string FILE_TERM_INDEX = "../lab6/term_index.bin";
const std::string FILE_POSTINGS = "../lab6/postings.bin";

struct TermEntry {
    std::string term;
    uint64_t offset;
    uint32_t count;
    
    bool operator<(const TermEntry& other) const {
        return term < other.term;
    }
};

struct DocResult {
    uint32_t id;
    std::string url;
    std::string title;
};

std::vector<TermEntry> dictionary;
std::ifstream f_postings;
std::ifstream f_docs_data;
std::ifstream f_docs_index;

void load_dictionary() {
    std::ifstream f(FILE_TERM_INDEX, std::ios::binary);
    if (!f.is_open()) return;

    while (f.peek() != EOF) {
        uint16_t len;
        if (!f.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
        
        std::string term(len, ' ');
        f.read(&term[0], len);
        
        uint64_t offset;
        uint32_t count;
        f.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        f.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        dictionary.push_back({term, offset, count});
    }
}

std::vector<uint32_t> get_postings(const std::string& term) {
    TermEntry target; 
    target.term = term;
    
    auto it = std::lower_bound(dictionary.begin(), dictionary.end(), target);
    
    if (it != dictionary.end() && it->term == term) {
        std::vector<uint32_t> result(it->count);
        
        f_postings.seekg(it->offset);
        f_postings.read(reinterpret_cast<char*>(result.data()), result.size() * sizeof(uint32_t));
        return result;
    }
    return {};
}

DocResult get_doc_info(uint32_t doc_id) {
    DocResult res;
    res.id = doc_id;
    
    f_docs_index.seekg(doc_id * 8);
    uint64_t offset;
    if (!f_docs_index.read(reinterpret_cast<char*>(&offset), sizeof(offset))) return res;
    
    f_docs_data.seekg(offset);
    
    uint16_t url_len;
    f_docs_data.read(reinterpret_cast<char*>(&url_len), sizeof(url_len));
    res.url.resize(url_len);
    f_docs_data.read(&res.url[0], url_len);
    
    uint16_t title_len;
    f_docs_data.read(reinterpret_cast<char*>(&title_len), sizeof(title_len));
    res.title.resize(title_len);
    f_docs_data.read(&res.title[0], title_len);
    
    return res;
}

std::vector<uint32_t> intersect(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    std::vector<uint32_t> res;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) i++;
        else if (b[j] < a[i]) j++;
        else {
            res.push_back(a[i]);
            i++; j++;
        }
    }
    return res;
}

std::vector<uint32_t> unite(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    std::vector<uint32_t> res;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) { res.push_back(a[i]); i++; }
        else if (b[j] < a[i]) { res.push_back(b[j]); j++; }
        else { res.push_back(a[i]); i++; j++; }
    }
    while (i < a.size()) res.push_back(a[i++]);
    while (j < b.size()) res.push_back(b[j++]);
    return res;
}

// A \ B - удаляем из A всё, что есть в B
// Унарный ! (!a) в поиске равносилен "все_доки \ a".
std::vector<uint32_t> exclude(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    std::vector<uint32_t> res;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) { res.push_back(a[i]); i++; }
        else if (b[j] < a[i]) { j++; }
        else { i++; j++; }
    }
    while (i < a.size()) res.push_back(a[i++]);
    return res;
}

struct Parser {
    std::string query;
    size_t pos = 0;

    Parser(std::string q) : query(q) {}

    char peek() {
        while (pos < query.size() && query[pos] == ' ') pos++;
        if (pos == query.size()) return 0;
        return query[pos];
    }

    std::string get_word() {
        while (pos < query.size() && !isalnum(query[pos]) && query[pos] != '_') pos++;
        std::string word;
        while (pos < query.size() && (isalnum(query[pos]) || query[pos] == '_')) {
            word += tolower(query[pos]);
            pos++;
        }
        return word;
    }

    std::vector<uint32_t> parse_factor() {
        char c = peek();
        if (c == '!') {
            pos++;
            
            auto to_exclude = parse_factor();
            // TODO: реализовать инверсию относительно всего корпуса
            // Пока вернем пустой, так как одиночный !word редко ищут
            return {}; 
        } 
        else if (c == '(') {
            pos++;
            auto res = parse_expression();
            if (peek() == ')') pos++;
            return res;
        } 
        else {
            return get_postings(get_word());
        }
    }
    
    std::vector<uint32_t> parse_term() {
        auto lhs = parse_factor();
        while (true) {
            char c = peek();
            if (c == 0 || c == '|' || c == ')') break;
            
            if (c == '&' && query[pos+1] == '&') {
                pos += 2;
                auto rhs = parse_factor();
                lhs = intersect(lhs, rhs);
            } else if (c == '!') {
                // a !b => a \ b
                pos++;
                auto rhs_raw = parse_factor();
                lhs = exclude(lhs, rhs_raw);
            } else {
                auto rhs = parse_factor();
                if (!rhs.empty())
                    lhs = intersect(lhs, rhs);
            }
        }
        return lhs;
    }

    std::vector<uint32_t> parse_expression() {
        auto lhs = parse_term();
        while (peek() == '|') {
            pos++;
            if (peek() == '|') pos++;
            auto rhs = parse_term();
            lhs = unite(lhs, rhs);
        }
        return lhs;
    }
};

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    
    if (argc < 2) {
        std::cerr << "Usage: ./search_engine \"query string\"" << std::endl;
        return 1;
    }

    std::string query = argv[1];

    f_postings.open(FILE_POSTINGS, std::ios::binary);
    f_docs_data.open(FILE_DOCS_DATA, std::ios::binary);
    f_docs_index.open(FILE_DOCS_INDEX, std::ios::binary);

    if (!f_postings || !f_docs_data || !f_docs_index) {
        std::cerr << "err: no index" << std::endl;
        return 1;
    }

    load_dictionary();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    Parser parser(query);
    std::vector<uint32_t> result_ids = parser.parse_expression();
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "{" << std::endl;
    std::cout << "  \"count\": " << result_ids.size() << "," << std::endl;
    std::cout << "  \"time_sec\": " << elapsed.count() << "," << std::endl;
    std::cout << "  \"results\": [" << std::endl;
    
    int limit = 50;
    for (size_t i = 0; i < result_ids.size() && i < limit; ++i) {
        DocResult doc = get_doc_info(result_ids[i]);
        
        std::string safe_title = doc.title; 
        std::replace(safe_title.begin(), safe_title.end(), '"', '\'');
        
        std::cout << "    { \"id\": " << doc.id << ", \"url\": \"" << doc.url 
                  << "\", \"title\": \"" << safe_title << "\" }";
        
        if (i < result_ids.size() - 1 && i < limit - 1) std::cout << ",";
        std::cout << std::endl;
    }
    std::cout << "  ]" << std::endl;
    std::cout << "}" << std::endl;

    return 0;
}