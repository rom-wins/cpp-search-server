#include "../include/remove_duplicates.h"
#include <iostream>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    vector<int> all_doc_ids(search_server.begin(), search_server.end());

    map<string, set<int>> word_to_docs;

    map<int, int> doc_id_to_count_words;

    for (int doc_id : all_doc_ids)
    {
        map<string, double> words_freqs = search_server.GetWordFrequencies(doc_id);
        doc_id_to_count_words[doc_id] = words_freqs.size();
        
        map<int, int> doc_to_count_match_words;

        for (const auto& [word, _] : words_freqs)
        {
            if(word_to_docs.count(word) > 0)
            {
                for (auto idid : word_to_docs.at(word))
                {
                    doc_to_count_match_words[idid]++;
                }
            }
            word_to_docs[word].emplace(doc_id);
        }

        for (const auto& [id, count] : doc_to_count_match_words)
        {
            if (doc_id_to_count_words.at(id) == words_freqs.size() && count == words_freqs.size())
            {
                cout << "Found duplicate document id "s << doc_id << endl; 
                search_server.RemoveDocument(max(doc_id, id));
                break;
            }
        }
    }
}

