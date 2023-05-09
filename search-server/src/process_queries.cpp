#include <algorithm>
#include <iterator>
#include <vector>
#include <list>
#include <numeric>
#include <string>
#include <execution>
#include "../include/search_server.h"
#include "../include/document.h"

using namespace std;

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> result(queries.size());

    transform(
            std::execution::par,
            queries.begin(), queries.end(),
            result.begin(),
            [&search_server](const string& query) { return search_server.FindTopDocuments(query);}
            );
    return result;
}


list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    auto docs_by_queries = ProcessQueries(search_server, queries);
    
    list<Document> result;
    for (int i = 0; i < docs_by_queries.size(); ++i)
    {
        std::move(docs_by_queries[i].begin(), docs_by_queries[i].end(), std::back_insert_iterator(result));
    }
    return result;
}

