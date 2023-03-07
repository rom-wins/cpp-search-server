#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <stdexcept>
#include <numeric>

using namespace std;

constexpr double RELEVANCE_ERROR = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
    Document() = default;

    Document(int id, double relevance, int rating)
            : id(id)
            , relevance(relevance)
            , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
                for (const string& word : MakeUniqueNonEmptyStrings(stop_words)){
                    if (!IsValidWord(word))
                    {
                        throw invalid_argument("Одно из стоп-слов содержит спецсимволы"s);
                    }
                }
    }

    explicit SearchServer(const string& stop_words_text)
            : SearchServer(
            SplitIntoWords(stop_words_text)) {
    }

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        
        if (document_id < 0)
        {
            throw invalid_argument("ID документа должен быть положительным"s);
        }

        if (documents_.count(document_id) > 0)
        {
            throw invalid_argument("Документ с таким ID уже добавлен"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);

        for (const auto& word : words )
        {
            if (!IsValidWord(word))
            {
                throw invalid_argument("Документ не должен содержать спецсимволы"s);
            }
        }

        const double inv_word_count = 1.0 / static_cast<int>(words.size());
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, GetDocumentCount()});
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const { 

        Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < RELEVANCE_ERROR) {
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

    vector<Document> FindTopDocuments(const string& raw_query,
                                               DocumentStatus status) const {
        return FindTopDocuments(
                raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                    return document_status == status;
                });
    }
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }


    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        
        Query query = ParseQuery(raw_query);
        
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        return make_tuple(matched_words, documents_.at(document_id).status);

    }

    int GetDocumentId(int index) const {
        if (index > -1 && index < GetDocumentCount())
        {
            for (const auto& [doc_id, doc_data] : documents_)
            {
                if (doc_data.number == index)
                {
                    return doc_id;
                }
            }
        }
        else {
            throw out_of_range("Индекс документа вне допустимого диапазона"s); 
        }

        return INVALID_DOCUMENT_ID;
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        int number;
    };
const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word)
    {
        return none_of(word.begin(), word.end(), [](char t){return t >= '\0' && t < ' ';});
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

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

     QueryWord ParseQueryWord(string text) const {
         
         bool is_minus = false;

         if (!IsValidWord(text))
         {
             throw invalid_argument("Слово запроса не должно содержать спецсимволы"s);
         }
        
         if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
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

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        
        Query query;

        for (const string& word : SplitIntoWords(text)) {
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

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                    {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

template <typename Func>
void RunTestImpl(const Func& func, const string& func_name) {
    func();
    cerr << func_name + " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {

    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const string stop_words = "in the"s;
    const string query = "in"s;
    const vector<int> ratings = {1, 2, 3};
    
    // Проверка корректности работы с пустой строкой стоп-слов
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(query);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    
    // Проверка исключения документа при наличии стоп-слов в нём
    {

        SearchServer server(stop_words);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments(query).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

// Тест проверяет, что поисковая система добавляет документы
void TestAddDocument() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const string query = "cat in";

    // убеждаемся что при создании поисковой системы документов нет
    {
        SearchServer server(""s);
        ASSERT_HINT(server.GetDocumentCount() == 0, "Сервер создан с уже добавленными документами"s);
    }

    // убеждаемся что при добавлении документа их количество внутри класса увеличивается
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.GetDocumentCount() == 1, "В сервер был добавлен 1 документ");
        server.AddDocument(doc_id + 1, content, DocumentStatus::BANNED, ratings);
        ASSERT_HINT(server.GetDocumentCount() == 2, "В сервер был добавлено 2 документа");
    }

    // Убеждаемся что добавленный документ находится по поисковому запросу
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const vector<Document> top_docs = server.FindTopDocuments(query);

        const int documents_with_id = static_cast<int>(count_if(top_docs.begin(), top_docs.end(), [doc_id](const Document& doc){
            return doc.id == doc_id;
        }));

        ASSERT_HINT(documents_with_id == 1, "Добавленный документ не находится по поисковому запросу"s);

    }
}

// Тест для минус-слов
void TestMinusWords() {

    const string& doc_1 = "cat in the city now"s;
    const int doc_id_1 = 0;

    const string& doc_2 = "cat in the kingdom"s;
    const int doc_id_2 = 1;

    const vector<int> ratings_docs {1, 2, 3};
    const DocumentStatus status = DocumentStatus::ACTUAL;


    const string& query = "cat in -city -now"s;

    // Убеждаемся что документы содержащие минус-слова поиского запроса не включаются в результаты поиска
    {
        SearchServer server(""s);
        server.AddDocument(doc_id_1, doc_1, status, ratings_docs);
        server.AddDocument(doc_id_2, doc_2, status, ratings_docs);

        const vector<Document> top_docs = server.FindTopDocuments(query);

        const int count_of_docs_with_id_1 = count_if(top_docs.begin(), top_docs.end(), [](const Document& doc){
            return doc.id == doc_id_1;
        });

        ASSERT_HINT(count_of_docs_with_id_1 == 0, "В результате поиска не должно быть данного документа, так как в нём содержится минус-слово"s);

        const int count_of_docs_with_id_2 = count_if(top_docs.begin(), top_docs.end(), [](const Document& doc){
            return doc.id == doc_id_2;
        });

        ASSERT_HINT(count_of_docs_with_id_2 == 1, "В результате поиска должнен быть данный документ, так как в нём не содержится минус-слово"s);

    }
}

// Тест для матчинга документов
void TestMatchDocument() {
    const int doc_id = 42;
    const string& content = "cat in the city"s;

    const vector<int> ratings = {1, 2, 3};
    const DocumentStatus status = DocumentStatus::ACTUAL;

    const string& match_query = "cat in"s;
    const int count_of_words_in_match_query = 2;

    const string& dont_match_query = "dog under kingdom"s;


    // Убеждаемся что возвращаются все слова из запроса, которые матчатся со словами документа
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, status, ratings);
        const auto& matched_docs = server.MatchDocument(match_query, doc_id);
        ASSERT_EQUAL_HINT(get<0>(matched_docs).size(),
                          count_of_words_in_match_query,
                          "Количество слов после матчинга должно быть равно "s + to_string(count_of_words_in_match_query));
    }

    // Убеждаемся что возвращается пустой список при матчинге документа с запросом содержащим минус слово
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, status, ratings);
        const auto& matched_docs = server.MatchDocument(dont_match_query, doc_id);
        ASSERT_HINT(get<0>(matched_docs).empty(),
                    "Вектор слов после матчинга должен быть пустым, так как запрос не содержит слов из документа"s);
    }
}

// Тест для сортировки найденных документов по релевантности
void TestSortingByRelevance() {

    // Убеждаемся что результат поиска топа документов отсортирован по убыванию
    {
        SearchServer server(""s);
        server.AddDocument(0, "big fat cat"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(1, "git fat cat"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(2, "cat little mouse"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(3, "soba boba aboba"s, DocumentStatus::ACTUAL, {1, 2, 3});

        const auto& top_docs = server.FindTopDocuments("big fat cat"s);

        for (int i = 0; i < top_docs.size()-1; ++i)
        {
            ASSERT_HINT(top_docs[i].relevance - top_docs[i+1].relevance >= RELEVANCE_ERROR,
                        "Результат поискового запроса не отсортирован по убыванию");
        }
    }
}

// Тест для предиката поиска по статусу
void TestStatusPredicate() {

    const string& doc_1 = "big fat cat"s;
    const string& doc_2 = "git fat cat"s;
    const string& doc_3 = "cat little mouse"s;
    const string& doc_4 = "soba boba aboba"s;


    const DocumentStatus status_1 = DocumentStatus::ACTUAL;
    const DocumentStatus status_2 = DocumentStatus::BANNED;
    const DocumentStatus status_3 = DocumentStatus::IRRELEVANT;
    const DocumentStatus status_4 = DocumentStatus::REMOVED;

    const vector<int> ratings = {1, 2, 3};

    SearchServer server(""s);
    server.AddDocument(0, doc_1, status_1, ratings);
    server.AddDocument(1, doc_2, status_2, ratings);
    server.AddDocument(2, doc_3, status_3, ratings);
    server.AddDocument(3, doc_4, status_4, ratings);

    // Поиск по статусу DocumentStatus::ACTUAL
    {
        const auto& top_docs = server.FindTopDocuments(doc_1, status_1);
        ASSERT_EQUAL_HINT(top_docs.size(), 1, "Результат поиска по статусу ACTUAL выполнен некорректно");
    }
    // Поиск по статусу DocumentStatus::BANNED
    {
        const auto& top_docs = server.FindTopDocuments(doc_2, status_3);
        ASSERT_EQUAL_HINT(top_docs.size(), 1, "Результат поиска по статусу BANNED выполнен некорректно");
    }

    // Поиск по статусу DocumentStatus::IRRELEVANT
    {
        const auto& top_docs = server.FindTopDocuments(doc_3, status_3);
        ASSERT_EQUAL_HINT(top_docs.size(), 1, "Результат поиска по статусу IRRELEVANT выполнен некорректно");
    }

    // Поиск по статусу DocumentStatus::REMOVED
    {
        const auto& top_docs = server.FindTopDocuments(doc_4, status_4);
        ASSERT_EQUAL_HINT(top_docs.size(), 1, "Результат поиска по статусу REMOVED выполнен некорректно");

    }
}

// Тест для правильности вычисления среднего рейтинга документа
void TestAverageRating() {

    const string& content = "big fat cat"s;
    const int doc_id = 0;
    const DocumentStatus status = DocumentStatus::ACTUAL;

    const auto create_server_and_get_top_docs = [&content](const vector<int>& ratings){
        SearchServer server(""s);
        const int average_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());

        server.AddDocument(doc_id, content, status, ratings);
        const auto& top_docs = server.FindTopDocuments(content);
        return make_pair(top_docs, average_rating);
    };

    // тест отрицательных и положительных рейтингов
    {
        const vector<int> ratings = {-50, -50, -50, 50, 50, 50};
        const auto& [top_docs, average_rating] = create_server_and_get_top_docs(ratings);
        ASSERT_EQUAL_HINT(top_docs[0].rating,
                          average_rating,
                          "Средний рейтинг из положительных и отрицательных оценок вычислен не верно");
    }

    // тест отрицательных рейтингов
    {
        const vector<int> ratings = {-50,-50,-50,-50,-50,-50};
        const auto& [top_docs, average_rating] = create_server_and_get_top_docs(ratings);
        ASSERT_EQUAL_HINT(top_docs[0].rating,
                          average_rating,
                          "Средний рейтинг из отрицательных оценок вычислен не верно");
    }

    // тест нулевых рейтингов
    {
        const vector<int> ratings = {0, 0, 0};
        const auto& [top_docs, average_rating] = create_server_and_get_top_docs(ratings);
        ASSERT_EQUAL_HINT(top_docs[0].rating,
                          average_rating,
                          "Средний рейтинг из нулевых оценок вычислен не верно");
    }

    // средний рейтинг из одного элемента
    {
        const vector<int> ratings = {1};
        const auto& [top_docs, average_rating] = create_server_and_get_top_docs(ratings);
        ASSERT_EQUAL_HINT(top_docs[0].rating,
                          average_rating,
                          "Средний рейтинг из одной оценки вычислен не верно");
    }
}

// Тест для разных предиков
void TestAnyPredicates() {

    const string& content = "big fat cat"s;
    const DocumentStatus status = DocumentStatus::ACTUAL;
    const vector<vector<int>> ratings = {
            {1, 2, 3, 4, 5},
            {0},
            {3, 3, 3},
            {-3, -3, -3},
            {5, 10, 15, 20},
            {-5, -4, -3}
    };

    // Поиск по четности id
    {
        SearchServer server(""s);
        int N = 5;

        for (int i = 0; i < N; ++i)
        {
            server.AddDocument(i, content, status, ratings[0]);
        }
        const auto& top_docs = server.FindTopDocuments(content, [](
                int document_id,
                [[maybe_unused]] DocumentStatus status,
                [[maybe_unused]] int rating){
            return document_id % 2 == 0;
        });

        for (const auto& doc : top_docs)
        {
            ASSERT_HINT(doc.id % 2 == 0, "Документов с не чётным id в результате поиского запроса быть не должно"s);
        }
    }

    // Поиск по заданному рейтингу
    {
        const int target_rating = 3;

        SearchServer server(""s);
        for (int i = 0; i < ratings.size(); ++i)
        {
            server.AddDocument(i, content, status, ratings[i]);
        }
        const auto& top_docs = server.FindTopDocuments(content, [](
                [[maybe_unused]] int document_id,
                [[maybe_unused]] DocumentStatus status,
                int rating){
            return rating == target_rating;
        });

        for (const auto& doc : top_docs)
        {
            ASSERT_EQUAL_HINT(doc.rating, target_rating, "Документов с данным рейтингом в результате поиского запроса быть не должно"s);
        }
    }
}

// Тестирование правильности расчёта релеватности
void TestRelevanceValueIsCorrect()
{
    const DocumentStatus status = DocumentStatus::ACTUAL;
    const vector<int> rating = {
            {1, 2, 3, 4, 5},
    };
    const string& content1 = "cat with big furry tail"s;
    const string& content2 = "little dog without collar"s;

    SearchServer server(""s);
    server.AddDocument(1, content1, status, rating);
    server.AddDocument(2, content2, status, rating);


    // Запрос в точности совпадает с документом 1 -> релеватность == 5 * 0.2 * log(2)
    {
        const string& query = content1;
        const double target_relevance = 5 * 0.2 * log(2);

        const auto& top_docs = server.FindTopDocuments(query);

        ASSERT_HINT(abs(top_docs[0].relevance - target_relevance) <= RELEVANCE_ERROR,
               "Релевантность вычислена не верно для запроса полностью совпадающего с документом"s);
    }

    // В запросе нет слов из документов -> relevance == 0
    {
        const string& query = "22 33 45 4532"s;

        const auto& top_docs = server.FindTopDocuments(query);

        ASSERT_HINT(top_docs.empty(),
                    "Релевантность вычислена не верно для запроса, слова которого не содержатся в документе"s);
    }


    // В запросе имеются некоторые слова из документов -> 0 < relevance < 1
    {
        const string& query = "little dog with tail"s;
        const double target_relevance_doc_1 = log(2) * 2 * 0.25;
        const double target_relevance_doc_2 = log(2) * 2 * 0.2;

        const auto& top_docs = server.FindTopDocuments(query);


        ASSERT_HINT(abs(top_docs[0].relevance - target_relevance_doc_1) <= RELEVANCE_ERROR,
                    "Релевантность вычислена не верно для запроса, некоторые слова которого содержатся в документе"s);

        ASSERT_HINT(abs(top_docs[1].relevance - target_relevance_doc_2) <= RELEVANCE_ERROR,
                    "Релевантность вычислена не верно для запроса, некоторые слова которого содержатся в документе"s);
    }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestSortingByRelevance);
    RUN_TEST(TestStatusPredicate);
    RUN_TEST(TestAverageRating);
    RUN_TEST(TestAnyPredicates);
    RUN_TEST(TestRelevanceValueIsCorrect);
}


// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}

