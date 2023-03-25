#pragma once
#include <ostream>
#include <vector>
#include <stdexcept>

using namespace std::string_literals;

template <typename Iterator>
class IteratorRange{
public:
    IteratorRange(const Iterator begin, const Iterator end): begin_(begin), end_(end){}

    auto begin() const
    {
        return begin_;
    }

    auto end() const
    {
        return end_;
    }

    auto size() const
    {
        return distance(begin_, end_);
    }

private:
    const Iterator begin_;
    const Iterator end_;
};


template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator> iterator)
{
    for (auto iter = iterator.begin(); iter != iterator.end(); ++iter)
    {
        out << *iter;
    }

    return out;
}


template <typename Iterator>
class Paginator{
public:
    Paginator(Iterator beg, Iterator end, int page_size)
    {
        if (page_size <= 0)
        {
            throw std::invalid_argument("Размер страницы должен быть положительным"s);
        }
        
        int dist = distance(beg, end);
        int count = dist / page_size;

        int i = 0;
        while(i < count)
        {
            pages_.push_back(IteratorRange<Iterator>(beg, beg+page_size));
            beg+=page_size;
            ++i;
        }
        if (dist % page_size != 0)
        {
            pages_.push_back(IteratorRange<Iterator>(beg, end));
        }
        
    }

    auto size() const 
    {
        return pages_.size();
    }

    auto begin() const
    {
        return this->pages_.begin();
    }

    auto end() const
    {
        return this->pages_.end();
    }
private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template<typename Container>
auto Paginate(const Container& cont, int page_size)
{
   return Paginator(begin(cont), end(cont), page_size);
}

