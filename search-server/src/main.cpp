#include <iostream>
#include "../include/test_example_functions.h"

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
    return 0;
}