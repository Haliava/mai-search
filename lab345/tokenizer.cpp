#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <iomanip>

struct Stats {
    long long total_tokens = 0;
    long long total_length = 0;
    std::map<std::string, int> frequency;
};

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
        std::cerr << "err" << filename << std::endl;
        return;
    }

    std::string line;
    std::string current_token;
    
    while (std::getline(file, line)) {
        for (char c : line) {
            if (is_separator(c)) {
                if (!current_token.empty()) {
                    stats.total_tokens++;
                    stats.total_length += current_token.length();
                    stats.frequency[current_token]++;
                    current_token.clear();
                }
            } else {
                current_token += to_lower_char(c);
            }
        }
        if (!current_token.empty()) {
            stats.total_tokens++;
            stats.total_length += current_token.length();
            stats.frequency[current_token]++;
            current_token.clear();
        }
    }
}

int main() {
    std::string input_file = "corpus_plain.txt";
    std::string output_csv = "zipf_data.csv";
    Stats stats;

    std::cout << "starting..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    process_file(input_file, stats);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "--- Результаты ---" << std::endl;
    std::cout << "Время выполнения: " << elapsed.count() << " сек" << std::endl;
    std::cout << "Всего токенов: " << stats.total_tokens << std::endl;
    std::cout << "Уникальных термов: " << stats.frequency.size() << std::endl;
    if (stats.total_tokens > 0) {
        std::cout << "Средняя длина токена: " << (double)stats.total_length / stats.total_tokens << std::endl;
        
        std::ifstream in(input_file, std::ifstream::ate | std::ifstream::binary);
        long long file_size = in.tellg(); 
        double speed = (file_size / 1024.0) / elapsed.count();
        std::cout << "Скорость: " << speed << " KB/sec" << std::endl;
    }

    std::cout << "saving..." << std::endl;
    
    std::vector<std::pair<std::string, int>> sorted_terms(stats.frequency.begin(), stats.frequency.end());
    
    std::sort(sorted_terms.begin(), sorted_terms.end(), 
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        }
    );

    std::ofstream csv(output_csv);
    csv << "Rank,Term,Frequency\n";
    for (size_t i = 0; i < sorted_terms.size(); ++i) {
        csv << (i + 1) << "," << sorted_terms[i].first << "," << sorted_terms[i].second << "\n";
    }

    std::cout << "Готово. Данные сохранены в " << output_csv << std::endl;

    return 0;
}