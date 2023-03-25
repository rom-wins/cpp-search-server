#include <ostream>
#include "../include/document.h"

using namespace std;

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
                    : id(id),
                    relevance(relevance), 
                    rating(rating) {}


ostream& operator<<(ostream& out, const Document& doc)
{
    out << "{ document_id = "s << doc.id;
    out << ", relevance = "s << doc.relevance;
    out << ", rating = "s << doc.rating;
    out << " }"s;
    return out;
}
