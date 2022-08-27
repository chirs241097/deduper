#include "compressed_vector.hpp"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <stdexcept>

int main()
{
    compressed_vector<uint8_t, 3> cv;
    compressed_vector<uint8_t, 3> cv2;
    std::vector<uint8_t> v;
    srand(time(NULL));
    for (int i = 0; i < 100; ++i)
    {
        int r = rand() % 8;
        cv.push_back(r);
        v.push_back(r);
    }
    for (int i = 0; i < 100; ++i)
    {
        if (cv.get(i) != v[i])
        {
            printf("%u <=> %u @ %d\n", cv.get(i), v[i], i);
            throw std::runtime_error(std::to_string(__LINE__));
        }
    }
    for (int i = 0; i < 1000; ++i)
    {
        if (rand() % 3)
        {
            int r = rand() % 8;
            cv.push_back(r);
            v.push_back(r);
        }
        else
        {
            if (cv.back() != v.back())
                throw std::runtime_error(std::to_string(__LINE__));
            cv.pop_back();
            v.pop_back();
        }
    }
    if (cv.size() != v.size())
        throw std::runtime_error(std::to_string(__LINE__));
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (cv.get(i) != v[i])
        {
            printf("%u <=> %u @ %lu\n", cv.get(i), v[i], i);
            throw std::runtime_error(std::to_string(__LINE__));
        }
        cv2.push_back(cv.get(i));
    }
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (cv.get(i) != cv2.get(i))
        {
            throw std::runtime_error(std::to_string(__LINE__));
        }
    }
    size_t h1 = compressed_vector_hash<uint8_t, 3>{}(cv);
    size_t h2 = compressed_vector_hash<uint8_t, 3>{}(cv2);
    if (h1 != h2)
    {
        printf("%lu <=> %lu\n", h1, h2);
        throw std::runtime_error(std::to_string(__LINE__));
    }
    return 0;
}
