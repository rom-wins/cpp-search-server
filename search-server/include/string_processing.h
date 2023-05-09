#pragma once
#include <string>
#include <unordered_set>
#include <string_view>
#include <vector>

std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::unordered_set<std::string_view, std::hash<std::string_view>, std::equal_to<std::string_view>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {

    std::unordered_set<std::string_view, std::hash<std::string_view>, std::equal_to<std::string_view>> non_empty_strings;
    for (std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
