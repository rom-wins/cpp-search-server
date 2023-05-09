#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include "../include/test_example_functions.h"
#include "../include/log_duration.h"
#include "../include/process_queries.h"
#include "../include/search_server.h"
#include "../include/benchmarks.h"

using namespace std;


int main() {
    
    cout << "---------------- Класс SearchServer: начало тестов ----------------"s << endl;
    TestSearchServer();
    cout << "----------- Класс SearchServer: тесты пройдены успешно ------------"s << endl;
    cout << endl;

    cout << "--------------- Класс Paginator: начало тестов ----------------"s << endl;
    TestPaginator();
    cout << "----------- Класс Paginator: тесты пройдены успешно ------------"s << endl;
    cout << endl;


    cout << "--------------- Класс RequestQueue: начало тестов ----------------"s << endl;
    TestRequestQueue();
    cout << "----------- Класс RequestQueue: тесты пройдены успешно ------------"s << endl;
    cout << endl;
    

    cout << "All tests were successful!"s << endl;
    
    cout << "-------------------- Начало запуска бенчмарков --------------------"s << endl;
    cout << "-------------------- BenchmarkFindTopDocument --------------------"s << endl;
    BenchmarkFindTopDocuments();
    cout << "-------------------- BenchmarkMatchDocument --------------------"s << endl;
    BenchmarkMatchDocument();
    cout << "-------------------- BenchmarkRemoveDocument --------------------"s << endl;
    BenchmarkRemoveDocument();
    cout << endl;

 
    return 0;
}


