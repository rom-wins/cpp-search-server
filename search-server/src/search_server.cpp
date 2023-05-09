#include <numeric>
#include <stdexcept>
#include <execution>
#include <iostream>
#include "../include/search_server.h"

using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}

void SearchServer::AddDocument(int document_id, 
                                string_view document, 
                                DocumentStatus status, 
                                const vector<int>& ratings) {
    
    if (document_id < 0)
    {
        throw invalid_argument("ID документа должен быть положительным"s);
    }

    if (documents_.count(document_id) > 0)
    {
        throw invalid_argument("Документ с таким ID уже добавлен"s);
    }

    vector<string_view> words = SplitIntoWordsNoStop(document);
    map<string, double, std::less<>> words_freeqs;
    
    const double inv_word_count = 1.0 / static_cast<int>(words.size());
    
    for (string_view word : words){
        word_to_document_freqs_[std::string{word}][document_id] += inv_word_count;
        words_freeqs[std::string{word}] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    doc_ids_.insert(document_id);
    
    doc_to_words_freeqs_[document_id] = words_freeqs;
}

vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, 
                                                DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
   
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word)
{
    return none_of(word.begin(), word.end(), [](char t){return t >= '\0' && t < ' ';});
}

    
vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    
    vector<string_view> splited_by_words = SplitIntoWords(text);

    for (string_view word : splited_by_words) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
        if (!IsValidWord(word))
        {
            throw invalid_argument("Документ не должен содержать спецсимволы"s);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    bool is_minus = false;

    if (!IsValidWord(text))
    {
        throw invalid_argument("Слово запроса не должно содержать спецсимволы"s);
    }
            
    if (text[0] == '-') {
        is_minus = true;
        
        text.remove_prefix(1);
        if (text.empty())
        {
            throw invalid_argument("После знака \"-\" в запросе должно быть минус слово"s);
        }
        else if (text[0] == '-')
        {
            throw invalid_argument("Использование выражения \"--\" в запросе недопустимо"s);
        }
    }

    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(string_view text) const {
        
    Query query;
    vector<string_view> splited_by_words = SplitIntoWords(text);
    
    for (string_view word : splited_by_words) {
        QueryWord query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    
    return query;
}


double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / (word_to_document_freqs_.find(word)->second).size());
}

const map<string, double, std::less<>>& SearchServer::GetWordFrequencies(int document_id) const {

    if (doc_to_words_freeqs_.count(document_id) > 0)
    {
        return doc_to_words_freeqs_.at(document_id);
    }

    static map<string, double, std::less<>> empty_map;
    return empty_map;
}

void SearchServer::RemoveDocument(int document_id)
{
    if (doc_ids_.count(document_id) < 1)
    {
        return;
    }

    for (auto& [word, _] : doc_to_words_freeqs_.at(document_id))
    {
        (word_to_document_freqs_.find(word)->second).erase(document_id);
    }

    documents_.erase(document_id);
    doc_to_words_freeqs_.erase(document_id); 
    doc_ids_.erase(document_id);
}


void SearchServer::RemoveDocument(std::execution::sequenced_policy, int document_id)
{
    return RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy, int document_id)
{
    
    if (doc_ids_.count(document_id) < 1)
    {
        return;
    }
    
    const auto& words_freeqs = doc_to_words_freeqs_.at(document_id);        
    vector<pair<string, double>> words_freeqs_v(words_freeqs.begin(), words_freeqs.end());
    
    for_each(
        std::execution::par,
        words_freeqs_v.begin(), words_freeqs_v.end(),
        [this, document_id](const pair<string, double>& item){ 
            (word_to_document_freqs_.find(item.first)->second).erase(document_id);
        }
    );

    documents_.erase(document_id);
    doc_ids_.erase(document_id);
    doc_to_words_freeqs_.erase(document_id);

}

