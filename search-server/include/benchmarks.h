#pragma once
#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "search_server.h"
#include "log_duration.h"


std::string GenerateWord(std::mt19937& generator, int max_length);

std::vector<std::string> GenerateDictionary(std::mt19937& generator, int word_count, int max_length);

std::string GenerateQuery(std::mt19937& generator, const std::vector<std::string>& dictionary, int word_count, double minus_prob);

std::vector<std::string> GenerateQueries(std::mt19937& generator, const std::vector<std::string>& dictionary, int query_count, int max_word_count);

void BenchmarkFindTopDocuments();

void BenchmarkMatchDocument();

void BenchmarkRemoveDocument();

template <typename ExecutionPolicy>
void LogDurationMatchDocument(const std::string& mark, const SearchServer& search_server, const std::string& query, const ExecutionPolicy& policy) {

    LOG_DURATION(mark + " MatchDocument"s);
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for (int id = 0; id < document_count; ++id) {
        const auto [words, status] = search_server.MatchDocument(policy, query, id);
        word_count += words.size();
    }
    std::cout << word_count << std::endl;

}

template <typename ExecutionPolicy>
void LogDurationFindTopDocuments(const std::string& mark, const SearchServer& search_server, const std::vector<std::string>& queries, const ExecutionPolicy& policy) {
    
    LOG_DURATION(mark + " FindTopDocuments"s);

    double total_relevance = 0;
    for (std::string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    std::cout << total_relevance << std::endl;
}

template <typename ExecutionPolicy>
void LogDurationRemoveDocument(const std::string& mark, SearchServer search_server, const ExecutionPolicy& policy) {
    LOG_DURATION(mark + " RemoveDocument"s);

    const int document_count = search_server.GetDocumentCount();
    for (int id = 0; id < document_count; ++id) {
        search_server.RemoveDocument(policy, id);
    }
    std::cout << "Result: "s << search_server.GetDocumentCount() << std::endl;
}

