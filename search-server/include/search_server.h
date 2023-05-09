#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <tuple>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <execution>
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

using namespace std::string_literals;

constexpr double RELEVANCE_ERROR = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    
    SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, 
                    std::string_view document, 
                    DocumentStatus status, 
                    const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const ;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, 
                                           std::string_view raw_query, 
                                           DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, 
                                           std::string_view raw_query, 
                                           DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, 
                                           std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, 
                                                                        int document_id) const;
    template<typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const ExecutionPolicy& policy, 
                                                                            std::string_view raw_query, 
                                                                            int document_id) const;
    
    int GetDocumentId(int index) const;

    const std::map<std::string, double, std::less<>>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id); 
    void RemoveDocument(std::execution::sequenced_policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy, int document_id);

    auto begin() const
    {
        return doc_ids_.begin();
    }

    auto end() const
    {
        return doc_ids_.end();
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    const std::unordered_set<std::string_view, std::hash<std::string_view>, std::equal_to<std::string_view>> stop_words_;
    
    std::set<int> doc_ids_;
    
    std::map<int, std::map<std::string, double, std::less<>>> doc_to_words_freeqs_;

    std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
    
    std::map<int, DocumentData> documents_;

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;
    
    struct Query {
        std::unordered_set<std::string_view, std::hash<std::string_view>, std::equal_to<std::string_view>> plus_words;
        std::unordered_set<std::string_view, std::hash<std::string_view>, std::equal_to<std::string_view>> minus_words;
    };

    Query ParseQuery(std::string_view text) const;
    
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, 
                                           DocumentPredicate document_predicate) const;
    
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, 
                                           const Query& query, 
                                           DocumentPredicate document_predicate) const;
    

};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) 
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    for (std::string_view word : stop_words_)
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument("Одно из стоп-слов содержит спецсимволы"s);
        }
    }
}

template<typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const ExecutionPolicy& policy,
                                                                       std::string_view raw_query, 
                                                                       int document_id) const {   

    auto words_freqs_it = doc_to_words_freeqs_.find(document_id);
        
    if (words_freqs_it == doc_to_words_freeqs_.end()) {
        throw std::out_of_range("Документа с данным id не существует.");
    }
    
    const auto& words_freeqs = words_freqs_it->second;
    
    Query query = ParseQuery(raw_query);
    
    auto find_minus_words = [&words_freeqs](std::string_view word){
        return words_freeqs.find(word) != words_freeqs.end();
    };


    bool has_minus_words = std::any_of(policy,
                                        query.minus_words.begin(), query.minus_words.end(),
                                        find_minus_words);    

    if (has_minus_words)
    {
        static std::vector<std::string_view> empty_vector;

        return {empty_vector, documents_.at(document_id).status};
 
    }
    
    std::vector<std::string_view> matched_words;
    for (std::string_view word : query.plus_words) {
        
        if (words_freeqs.find(word) != words_freeqs.end())
        {
            matched_words.push_back(word);
        }
    }
    
    return {matched_words, documents_.at(document_id).status};
}



template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, 
                                                    DocumentPredicate document_predicate) const 
{
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, 
                                                     std::string_view raw_query, 
                                                     DocumentPredicate document_predicate) const
{

    Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(
        policy,
        matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) 
            {
                if (std::abs(lhs.relevance - rhs.relevance) < RELEVANCE_ERROR) 
                {
                    return lhs.rating > rhs.rating;
                } else {
                     return lhs.relevance > rhs.relevance;
                }
             });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, 
                                                     std::string_view raw_query, 
                                                     DocumentStatus status) const
{

    return FindTopDocuments(
        policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, 
                                                     std::string_view raw_query) const
{
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query,
                                                    DocumentPredicate document_predicate) const 
{
    return FindAllDocuments(std::execution::seq, query, document_predicate);
}


template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy,
                                                     const SearchServer::Query& query,
                                                     DocumentPredicate document_predicate) const 
{
    int TREAD_NUM = 1;
    bool vectorize_plus_words = false;
    if constexpr (std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>)
    {
        TREAD_NUM = 10;
        vectorize_plus_words = true;
    }

    ConcurrentMap<int, double> conc_map(TREAD_NUM);
    
    auto get_docs_by_plus_word = [this, &document_predicate, &conc_map](std::string_view word) {
                                    
        auto doc_freqs_it = word_to_document_freqs_.find(word);
        if (doc_freqs_it == word_to_document_freqs_.end()) {
            return;
        }
        const std::map<int, double>& doc_freqs = doc_freqs_it->second;
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : doc_freqs) 
        {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) 
            {
                conc_map[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
    };

    if(vectorize_plus_words)
    {
        std::vector<std::string_view> v_plus_words(query.plus_words.begin(), query.plus_words.end());
        std::for_each(
            policy,
            v_plus_words.begin(), v_plus_words.end(),
            get_docs_by_plus_word
        );
    }
    else
    {
        std::for_each(
            policy,
            query.plus_words.begin(), query.plus_words.end(),
            get_docs_by_plus_word
        );
    }

    std::map<int, double> document_to_relevance = conc_map.BuildOrdinaryMap();   
    
    for_each(
        query.minus_words.begin(), query.minus_words.end(),
        [this, &document_to_relevance](std::string_view word){
            auto doc_freqs_it = word_to_document_freqs_.find(word);
        
            if (doc_freqs_it == word_to_document_freqs_.end()) {
                return;
            }
            const auto& doc_freqs = doc_freqs_it->second;
        
            for (const auto [document_id, _] : doc_freqs) 
            {
                document_to_relevance.erase(document_id);
            }
 
        }
    );

    std::vector<Document> matched_documents;
        
    for (const auto [document_id, relevance] : document_to_relevance) 
    {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

