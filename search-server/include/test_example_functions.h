#pragma once
#include <stdexcept>
#include <iostream>
#include <string>

using namespace std::string_literals;

template <typename Func>
void RunTestImpl(const Func& func, const std::string& func_name) {
    func();
    std::cerr << func_name + " OK"s << std::endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

template <typename T, typename U>
void AssertEqualImpl(const T& t, 
                    const U& u, 
                    const std::string& t_str, 
                    const std::string& u_str, 
                    const std::string& file,
                    const std::string& func, 
                    unsigned line, 
                    const std::string& hint) {

    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, 
                const std::string& expr_str,
                const std::string& file,
                const std::string& func,
                unsigned line,
                const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();

// Тест проверяет, что поисковая система добавляет документы
void TestAddDocument();

// Тест для минус-слов
void TestMinusWords();

// Тест для матчинга документов
void TestMatchDocument() ;

// Тест для сортировки найденных документов по релевантности
void TestSortingByRelevance();

// Тест для предиката поиска по статусу
void TestStatusPredicate();

// Тест для правильности вычисления среднего рейтинга документа
void TestAverageRating();

// Тест для разных предиков
void TestAnyPredicates();

// Тестирование правильности расчёта релеватности
void TestRelevanceValueIsCorrect();

// Тестирование правильности удаления документа
void TestRemoveDocument();

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();

// Тестирование функции удаления дубликатов из SearchServer
void TestDuplicates();

// --------- Окончание модульных тестов поисковой системы -----------


// --------- Начало модульных тестов постраничной выдачи ------------

// Функция проверяет правильность размера страницы при постраничном поиске
void TestPageSize();

void TestPaginator();
// -------- Окончание модульных тестов постраничной выдачи ----------


// ----------- Начало модульных тестов очереди запросов -------------

// Функция проверяет правильность подсчёта запросов с нулевым результатом
void TestCountOfNoResultRequest();
 
// Функция проверяет правильность удаления устаревших запросов
void TestRemoveOldRequest();

// Функция является точкой входа для запуска тестов очереди запросов
void TestRequestQueue();
// --------- Окончание модульных тестов очереди запросов ------------


