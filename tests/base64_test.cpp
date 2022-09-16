#include "base64.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <ctime>

char buf[32768];
char bug[32768];
char buh[32768];
char bui[32768];

void testb64class()
{
    srand(time(NULL));
    size_t l1 = rand() % 20 + 1;
    size_t l2 = rand() % 20 + 1;
    for (size_t i = 0; i < l1; ++i)
        buf[i] = rand() % 128;
    for (size_t i = 0; i < l2; ++i)
        bug[i] = rand() % 128;
    base64_encoder enc;
    enc.encode_data(buf, l1);
    enc.encode_data(bug, l2);
    std::string s = enc.finalize();
    std::string ss = enc.finalize();
    base64_decoder dec(std::move(s));
    assert(dec.decoded_length() == l1 + l2);

    base64_decoder decc(std::move(s));
    size_t xx = decc.decode_data(buh, 32768);
    for (size_t i = 0; i < xx; ++i)
        printf("%d ", buh[i]);
    printf("\n");
    size_t l3 = dec.decode_data(buh, l1);
    size_t l4 = dec.decode_data(bui, l2);
    assert(l1 == l3);
    assert(l2 == l4);
    for (size_t i = 0; i < l1 ; ++i)
        printf("%d ", buf[i]);
    printf("\n");
    for (size_t i = 0; i < l1 ; ++i)
        printf("%d ", buh[i]);
    printf("\n");fflush(stdout);
    assert(!memcmp(buf, buh, l1));
    for (size_t i = 0; i < l2 ; ++i)
        printf("%d ", bug[i]);
    printf("\n");
    for (size_t i = 0; i < l2 ; ++i)
        printf("%d ", bui[i]);
    printf("\n");fflush(stdout);
    assert(!memcmp(bug, bui, l2));
}

int main()
{
    /*freopen(NULL, "rb", stdin);
    size_t s = fread(buf, 1, 32768, stdin);
    std::string en = base64_encode((void*)buf, s);
    puts(en.c_str());
    size_t rl = 0;
    char *de = (char*)base64_decode(en, &rl);
    if (rl != s) return 1;
    if (memcmp(buf, de, s)) return 1;
    free(de);*/
    testb64class();
    return 0;
}
