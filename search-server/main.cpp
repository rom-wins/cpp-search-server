#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

struct Query{
    set<string> plus_words;
    set<string> minus_words;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }


    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double TF_word = 1.0 / words.size();
        for (const string& word : words)
        {
            word_to_document_freqs_[word][document_id] += TF_word;
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    void PrintDocumentCount() const
    {
        cout << document_count_ << endl;
    }

private:
    map<string, map<int, double>> word_to_document_freqs_ ;

    set<string> stop_words_;
    int document_count_ = 0;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }


    Query ParseQuery(const string& text) const {
        set<string> plus_query_words;
        set<string> minus_query_words;

        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-')
            {
                string new_word = word.substr(1);
                if (!IsStopWord(new_word))
                {
                    minus_query_words.insert(new_word);
                }
            }
            else
            {
                plus_query_words.insert(word);
            }
        }
        return Query{plus_query_words, minus_query_words};
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;

        // увеличение релевантности по плюс-словам
        for(const auto& word : query.plus_words)
        {
            if(word_to_document_freqs_.count(word) != 0)
            {
                const auto& documents =  word_to_document_freqs_.at(word);
                double IDF = log(document_count_ * 1.0 / documents.size());

                for(const auto [document_id, TF] : documents)
                {
                    document_to_relevance[document_id] += TF * IDF;
                }
            }

        }

        // удаление из результата по минус-словам
        for(const auto& word : query.minus_words)
        {
            if(word_to_document_freqs_.count(word) != 0)
            {
                for(const auto [document_id, TF] : word_to_document_freqs_.at(word))
                {
                    document_to_relevance.erase(document_id);
                }
            }
        }

        for(const auto& [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back({document_id, relevance});
        }

        return matched_documents;
    }

};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}