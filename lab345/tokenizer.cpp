#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <set>

const std::string INPUT_FILE = "corpus_plain.txt";
const std::string OUTPUT_CSV = "zipf_stemmed.csv";

struct Stats {
    long long total_tokens = 0;
    long long total_length = 0;
    std::map<std::string, int> frequency;
};

bool ends_with(const std::string& word, const std::string& suffix) {
    if (word.length() < suffix.length()) return false;
    return word.compare(word.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool contains_vowel(const std::string& word, size_t end_pos) {
    for (size_t i = 0; i < end_pos; ++i) {
        char c = word[i];
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y')
            return true;
    }
    return false;
}

std::string stem(std::string word) {
    if (word.length() <= 2) return word;

    if (ends_with(word, "sses")) word.pop_back(), word.pop_back();
    else if (ends_with(word, "ies")) { word.pop_back(); word.pop_back(); word += "i"; }
    else if (ends_with(word, "ss")) {}
    else if (ends_with(word, "s")) word.pop_back();

    bool extra_step = false;
    if (ends_with(word, "eed")) {
        if (word.length() > 4) word.pop_back();
    } else if (ends_with(word, "ed")) {
        std::string stem_candidate = word.substr(0, word.length() - 2);
        if (contains_vowel(stem_candidate, stem_candidate.length())) {
            word = stem_candidate;
            extra_step = true;
        }
    } else if (ends_with(word, "ing")) {
        std::string stem_candidate = word.substr(0, word.length() - 3);
        if (contains_vowel(stem_candidate, stem_candidate.length())) {
            word = stem_candidate;
            extra_step = true;
        }
    }

    if (extra_step) {
        if (ends_with(word, "at") || ends_with(word, "bl") || ends_with(word, "iz")) {
            word += "e";
        } else if (word.length() >= 2 && word[word.length()-1] == word[word.length()-2]) {
            char last = word.back();
            if (last != 'l' && last != 's' && last != 'z') {
                word.pop_back();
            }
        }
    }

    if (ends_with(word, "y")) {
        std::string stem_candidate = word.substr(0, word.length() - 1);
        if (contains_vowel(stem_candidate, stem_candidate.length())) {
            word = stem_candidate + "i";
        }
    }

    if (ends_with(word, "ational")) word.replace(word.length()-7, 7, "ate");
    else if (ends_with(word, "tional")) word.replace(word.length()-6, 6, "tion");
    else if (ends_with(word, "izer")) word.replace(word.length()-4, 4, "ize");
    else if (ends_with(word, "abli")) word.replace(word.length()-4, 4, "able");
    else if (ends_with(word, "alli")) word.replace(word.length()-4, 4, "al");
    else if (ends_with(word, "entli")) word.replace(word.length()-5, 5, "ent");
    else if (ends_with(word, "eli")) word.replace(word.length()-3, 3, "e");
    else if (ends_with(word, "ousli")) word.replace(word.length()-5, 5, "ous");

    return word;
}

bool is_separator(char c) {
    if ((unsigned char)c > 127) return false;
    if (isalnum(c)) return false;
    return true;
}

char to_lower_char(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

void process_file(const std::string& filename, Stats& stats) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening " << filename << std::endl;
        return;
    }

    std::string line;
    std::string current_token;
    
    while (std::getline(file, line)) {
        for (char c : line) {
            if (is_separator(c)) {
                if (!current_token.empty()) {
                    std::string stemmed = stem(current_token);
                    
                    stats.total_tokens++;
                    stats.total_length += stemmed.length();
                    stats.frequency[stemmed]++;
                    current_token.clear();
                }
            } else {
                current_token += to_lower_char(c);
            }
        }
        if (!current_token.empty()) {
            std::string stemmed = stem(current_token);
            stats.total_tokens++;
            stats.total_length += stemmed.length();
            stats.frequency[stemmed]++;
            current_token.clear();
        }
    }
}

int main() {
    Stats stats;
    std::cout << "starting..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    process_file(INPUT_FILE, stats);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "--- Results (with Stemming) ---" << std::endl;
    std::cout << "Time: " << elapsed.count() << " sec" << std::endl;
    std::cout << "Total Tokens: " << stats.total_tokens << std::endl;
    std::cout << "Unique Terms: " << stats.frequency.size() << std::endl;
    
    std::vector<std::pair<std::string, int>> sorted_terms(stats.frequency.begin(), stats.frequency.end());
    std::sort(sorted_terms.begin(), sorted_terms.end(), 
        [](const auto& a, const auto& b) { return a.second > b.second; }
    );

    std::ofstream csv(OUTPUT_CSV);
    csv << "Rank,Term,Frequency\n";
    for (size_t i = 0; i < sorted_terms.size(); ++i) {
        csv << (i + 1) << "," << sorted_terms[i].first << "," << sorted_terms[i].second << "\n";
    }
    std::cout << "Saved to " << OUTPUT_CSV << std::endl;

    return 0;
}