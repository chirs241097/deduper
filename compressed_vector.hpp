#ifndef COMPRESSED_VECTOR
#define COMPRESSED_VECTOR

#include <cstdint>
#include <cstddef>
#include <vector>

template <class T, int B>
struct compressed_vector_hash;

/*limitations:
 *  current implementation never returns a reference
 */
template <class T, int B>
class compressed_vector
{
    static_assert(std::is_unsigned<T>::value);
    static_assert(sizeof(T) * 8 >= B);
    static_assert(B > 0 && B < 64);
private:
    const int P = 64 / B;
    const uint64_t M = (1 << B) - 1;
    std::vector<uint64_t> v;
    size_t sz;
public:
    compressed_vector() : sz(0) {}
    void push_back(T val)
    {
        //assert(v <= M);
        if (sz % P == 0)
            v.push_back(0);
        set(sz, val);
        ++sz;
    }
    void pop_back()
    {
        //assert(sz > 0);
        if (--sz % P == 0)
            v.pop_back();
        else
        //zero out the now unused bits
            v[sz / P] &= ~(M << (sz % P * B));
    }
    T front() const {return get(0);}
    T back() const {return get(sz - 1);}
    T get(size_t i) const
    {
        //assert(i < sz);
        return (T)((v[i / P] >> (i % P * B)) & M);
    }
    void set(size_t i, T val)
    {
        //assert(i < sz);
        v[i / P] &= ~(M << (i % P * B));
        v[i / P] |= ((uint64_t) val) << (i % P * B);
    }
    size_t size() const
    {
        return sz;
    }
    void clear()
    {
        sz = 0;
        v.clear();
    }

    bool operator ==(const compressed_vector& other) const
    {
        return sz == other.sz && v == other.v;
    }

    friend struct compressed_vector_hash<T, B>;
};

template <class T, int B>
struct compressed_vector_hash
{
    size_t operator()(compressed_vector<T, B> const& s) const noexcept
    {
        size_t ret = 0;
        //Fibonacci hashing
        for (size_t i = 0; i < s.v.size(); ++i) {
            ret ^= (size_t)(s.v[i] & ~0U) + 0x9e3779b9 + (ret << 6) + (ret >> 2);
            ret ^= (size_t)(s.v[i] >> 32) + 0x9e3779b9 + (ret << 6) + (ret >> 2);
        }
        return ret;
    }
};

#endif
