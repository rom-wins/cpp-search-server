#pragma once
#include <map>
#include <vector>
#include <mutex>
#include <type_traits>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:

    struct Bucket {
        std::mutex mutex;
        std::map<Key, Value> my_map;

    };
    
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket) : guard(bucket.mutex), ref_to_value(bucket.my_map[key])
        {
        
        }
    };

    explicit ConcurrentMap(size_t bucket_count) : buckets_(bucket_count)
    {
    }

    Access operator[](const Key& key)
    {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return {key, bucket};
    }

    std::map<Key, Value> BuildOrdinaryMap()
    {
        if (buckets_.size() == 1)
        {
            return buckets_[0].my_map;
        }

        std::map<Key, Value> result;
        for (auto& [mut, my_map] : buckets_)
        {
            std::lock_guard g(mut);
            result.insert(my_map.begin(), my_map.end());
        }
        return result;
    }

private:
    std::vector<Bucket> buckets_;
};

