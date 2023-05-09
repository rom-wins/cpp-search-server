#include <string>
#include <vector>
#include "../include/search_server.h"
#include "../include/request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(&search_server) {}
    
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    return RequestQueue::AddFindRequest(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    return RequestQueue::AddFindRequest(raw_query, [](int document_id, DocumentStatus document_status, int rating){
                return document_status == DocumentStatus::ACTUAL;
            });
}
int RequestQueue::GetNoResultRequests() const {
    return RequestQueue::no_result_requests; 
}
void RequestQueue::RemoveOld()
{
    if (requests_.size() > min_in_day_)
    {
        if (requests_.front().count_of_results == 0)
        {
            --no_result_requests;
        }
        requests_.pop_front();
    }
}


