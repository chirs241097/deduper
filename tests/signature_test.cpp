#include <cstdio>
#include <vector>
#include "subslice_signature.hpp"
#include "signature.hpp"

signature_config cfg_full =
{
    9,      //slices
    3,      //blur_window
    2,      //min_window
    true,   //crop
    true,   //comp
    0.5,    //pr
    1./128, //noise_threshold
    0.05,   //contrast_threshold
    0.25    //max_cropping
};

signature_config cfg_subslice =
{
    4,      //slices
    16,     //blur_window
    2,      //min_window
    false,  //crop
    true,   //comp
    0.5,    //pr
    1./64,  //noise_threshold
    0.05,   //contrast_threshold
    0.25    //max_cropping
};

int main(int argc, char **argv)
{
    std::vector<subsliced_signature> a;
    for (int i = 1; i < argc; ++i)
        a.push_back(std::move(subsliced_signature::from_file(argv[i], 3, 3, cfg_full, cfg_subslice)));
    for (auto& ss : a)
    {
        for (auto& s : ss.subslices)
            printf("%lu ", signature_hash{}(s));
        puts("");
    }
    if (a.size() < 2) return 0;
    for (size_t i = 0; i < a.size(); ++i)
    for (size_t j = i + 1; j < a.size(); ++j)
    {
        printf("%lu <-> %lu:", i, j);
        double d = a[i].full.distance(a[j].full);
        printf("%f\n", d);
    }
    //while (cv::waitKey(0) != 'q');
    return 0;
}
