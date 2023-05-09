#pragma once
#include <string>
#include <vector>
#include <deque>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) ;
    
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const ;
private:
    struct QueryResult {
        uint64_t count_of_results;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer* search_server_;
    int no_result_requests = 0;

    void RemoveOld();
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate)
{
    const auto search_result = search_server_->FindTopDocuments(raw_query, document_predicate);
        
    requests_.push_back({search_result.size()});
        
    if (search_result.empty())
    {
        ++no_result_requests;
    }
        
    RemoveOld();

    return search_result;
}


