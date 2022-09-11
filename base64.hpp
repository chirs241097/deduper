#include <cstdint>
#include <string>

std::string base64_encode(const void *data, size_t len);
void* base64_decode(const std::string &s, size_t *rel);

class Base64Encoder
{
private:
    static const char *b64c;
    uint8_t counter;
    uint8_t rem;
    std::string ret;
public:
    Base64Encoder();
    void encode_data(const void *data, size_t len);
    std::string finalize();
};

class Base64Decoder
{
private:
    static const uint8_t b64v[];
    size_t dlen;
    bool invalid;
    uint8_t rem;
    uint8_t counter;
    size_t bp;
    std::string s;
public:
    Base64Decoder(std::string &&b);
    size_t decoded_length();
    size_t decode_data(const void *data, size_t len);
};
