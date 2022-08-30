#include <cstdio>
#include <vector>
#include "signature.hpp"
//#include <opencv2/highgui.hpp>
int main(int argc, char **argv)
{
    std::vector<signature> a;
    for (int i = 1; i < argc; ++i)
        a.push_back(std::move(signature::from_file(argv[i], signature::default_cfg())));
    if (a.size() < 2) return 0;
    for (size_t i = 0; i < a.size(); ++i)
    for (size_t j = i + 1; j < a.size(); ++j)
    {
        printf("%lu <-> %lu:", i, j);
        double d = a[i].distance(a[j]);
        printf("%f\n", d);
    }
    //while (cv::waitKey(0) != 'q');
    return 0;
}
