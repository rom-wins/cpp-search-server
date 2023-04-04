#include "../include/remove_duplicates.h"
#include <iostream>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {

    set<int> ids_to_del;
    set<set<string>> uniq_words_by_docs;
    set<string> all_words;

    for (auto doc_id_it = search_server.begin(); doc_id_it != search_server.end(); ++doc_id_it)
    {
        auto words_freqs = search_server.GetWordFrequencies(*doc_id_it);
        set<string> uniq_words;
        for (const auto& [word, _] : words_freqs)
        {
            if(all_words.count(word) > 0)
            {
                uniq_words.emplace(word);
            }
            else
            {
                all_words.emplace(word);
            }
        }
        
        if (!uniq_words.empty() || uniq_words_by_docs.count(uniq_words) > 0)
        {
            ids_to_del.emplace(*doc_id_it);
        }
        else
        {
            uniq_words_by_docs.emplace(uniq_words);
        }
    }
    for (int doc_id : ids_to_del)
    {
        cout << "Found duplicate document id "s << doc_id << endl; 
        search_server.RemoveDocument(doc_id);
    }
}

