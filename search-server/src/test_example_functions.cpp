#include <cmath>
#include <string>
#include <numeric>
#include <iostream>
#include <vector>
#include "../include/test_example_functions.h"
#include "../include/search_server.h"
#include "../include/request_queue.h"
#include "../include/paginator.h"

using namespace std;

void AssertImpl(bool value, 
                const string& expr_str, 
                const string& file, 
                const string& func, 
                unsigned line,
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


// --------- Начало модульных тестов постраничной выдачи ------------

// Функция проверяет правильность размера страницы при постраничном поиске
void TestPageSize()
{
    {
        SearchServer search_server("and with"s);
        search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
        search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
        search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
        search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});
        
        const auto search_results = search_server.FindTopDocuments("curly dog"s);
        int page_size = 2;
        
        const auto pages = Paginate(search_results, page_size);
        ASSERT_EQUAL_HINT(pages.size(), 
                            static_cast<int>(ceil(search_results.size() * 1.0 / page_size)),
                            "Количество страниц постраничного поиска некорректно"s);
        
        for (auto page = pages.begin(); page != pages.end(); ++page) {
            if (page + 1 != pages.end())
            {
                ASSERT_EQUAL_HINT(page->size(), page_size, "Количество документов на странице некорректно"s); 
            }
            else {
                ASSERT_EQUAL_HINT(page->size(), 
                                    search_results.size() % page_size, 
                                    "Количество документов последней страницы некорректно"s); 
            }
        }
    }
}

void TestPaginator()
{
    RUN_TEST(TestPageSize);
}
// -------- Окончание модульных тестов постраничной выдачи ----------


// ----------- Начало модульных тестов очереди запросов -------------

// Функция проверяет правильность подсчёта запросов с нулевым результатом
void TestCountOfNoResultRequest()
{
    int cout_of_queries_without_results = 11;
    int cout_of_queries_with_results = 15;


    {
        SearchServer search_server("and in at"s);
        RequestQueue request_queue(search_server);
        search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
        search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
        search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
        search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    
        for (int i = 0; i < cout_of_queries_without_results; ++i) {
            request_queue.AddFindRequest("empty request"s);
        }
        for (int i = 0; i < cout_of_queries_with_results; ++i)
        {
            request_queue.AddFindRequest("big dog"s);
        }
    
        ASSERT_EQUAL_HINT(request_queue.GetNoResultRequests(), 
                        cout_of_queries_without_results, 
                        "Очередь запросов неверно вычисляет количество запросов с нулевым результатом"s);

    }
}

// Функция проверяет правильность удаления устаревших запросов
void TestRemoveOldRequest()
{
    {
        SearchServer search_server("and in at"s);
        RequestQueue request_queue(search_server);
        search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
        search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
        search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
        search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    
        for (int i = 0; i < 1439; ++i) {
            request_queue.AddFindRequest("empty request"s);
        }
        // все еще 1439 запросов с нулевым результатом
        request_queue.AddFindRequest("curly dog"s);
        
        // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
        request_queue.AddFindRequest("big collar"s);
        
        // первый запрос удален, 1437 запросов с нулевым результатом
        request_queue.AddFindRequest("sparrow"s);
        
        ASSERT_EQUAL_HINT(request_queue.GetNoResultRequests(), 
                        1437, 
                        "Очередь запросов неверно вычисляет количество запросов с нулевым результатом"s);

    }
}

// Функция является точкой входа для запуска тестов очереди запросов
void TestRequestQueue()
{
    RUN_TEST(TestCountOfNoResultRequest);
    RUN_TEST(TestRemoveOldRequest);
}
// --------- Окончание модульных тестов очереди запросов ------------


