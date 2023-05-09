#include <string>
#include <vector>
#include <string_view>
#include "../include/string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> words;
    int last = -1;
    for (int i = 0; i < text.size(); ++i) {
        
        char c = text[i];
        if (c != ' ' && last == -1)
        {
            last = i;
            continue;
        }
        if (c == ' ' && last != -1)
        {
            std::string_view word(text.data() + last, i - last);
            words.push_back(word);
            last = -1;
        }
        
    }
    if (last != -1)
    {
        std::string_view word(text.data() + last, text.size() - last);
        words.push_back(word);
    }
    
    return words;
}

