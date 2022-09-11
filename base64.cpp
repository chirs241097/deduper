//Chris Xiong 2022
//License: MPL-2.0
#include <string>
#include <cstdint>
#include <cstdlib>

#include "base64.hpp"

const char *Base64Encoder::b64c = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

Base64Encoder::Base64Encoder() : counter(0), rem(0), ret(std::string()) {}

void Base64Encoder::encode_data(const void *data, size_t len)
{
    const uint8_t *u8d = (uint8_t*) data;
    for (size_t i = 0; i < len; ++i)
    {
        ++counter;
        if (counter == 3) counter = 0;
        switch (counter)
        {
            case 0:
                rem |= (u8d[i] >> 6);
                ret.push_back(b64c[rem]);
                ret.push_back(b64c[u8d[i] & 0b111111]);
            break;
            case 1:
                ret.push_back(b64c[u8d[i] >> 2]);
                rem = (u8d[i] & 0b11) << 4;
            break;
            case 2:
                rem |= (u8d[i] >> 4);
                ret.push_back(b64c[rem]);
                rem = (u8d[i] & 0b1111) << 2;
            break;
        }
    }
}

std::string Base64Encoder::finalize()
{
    if (counter)
    {
        ret.push_back(b64c[rem]);
        for (int i = 0; i < 3 - counter; ++i)
            ret.push_back('=');
    }
    return ret;
}

const uint8_t Base64Decoder::b64v[] = {
    65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
    65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
    65,65,65,65,65,65,65,65,65,65,65,62,65,65,65,63,
    52,53,54,55,56,57,58,59,60,61,65,65,65,64,65,65,
    65, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,65,65,65,65,65,
    65,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,65,65,65,65,65
};

Base64Decoder::Base64Decoder(std::string &&b) :
    s(b),
    invalid(false),
    rem(0),
    counter(0),
    bp(0)
{
    size_t npadd = 0;
    for (auto ri = s.rbegin(); ri != s.rend(); ++ri)
        if (*ri == '=')
            ++npadd;
        else break;
    dlen = (s.length() - npadd) / 4 * 3;
    switch (npadd)
    {
        case 0: break;
        case 1: dlen += 2; break;
        case 2: dlen += 1; break;
        default:
            dlen = 0;
            invalid = true;
    }
}

size_t Base64Decoder::decoded_length()
{
    return dlen;
}

size_t Base64Decoder::decode_data(const void *data, size_t len)
{
    uint8_t *rp = (uint8_t*)data;
    for (; bp < s.size(); ++bp)
    {
        ++counter;
        if (counter == 4) counter = 0;
        if (s[bp] == '=') break;
        if (s[bp] < 0 || b64v[s[bp]] > 64)
        {
            invalid = true;
            return 0;
        }
        switch (counter)
        {
            case 0:
                rem |= b64v[s[bp]];
                *(rp++) = rem;
            break;
            case 1:
                rem = b64v[s[bp]] << 2;
            break;
            case 2:
                rem |= b64v[s[bp]] >> 4;
                *(rp++) = rem;
                rem = (b64v[s[bp]] & 0b1111) << 4;
            break;
            case 3:
                rem |= b64v[s[bp]] >> 2;
                *(rp++) = rem;
                rem = (b64v[s[bp]] & 0b11) << 6;
            break;
        }
        if (rp - (uint8_t*)data == len)
        {
            ++bp;
            break;
        }
    }
    return rp - (uint8_t*)data;
}
