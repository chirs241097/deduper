#include <cstdio>
#include <vector>
#include "signature.hpp"
//#include <opencv2/highgui.hpp>
int main()
{
    std::vector<signature> a;
    a.push_back(std::move(signature::from_file("img/x.jpg")));
    a.push_back(std::move(signature::from_file("img/z.jpg")));
    for (size_t i = 0; i < a.size(); ++i)
    for (size_t j = 0; j < a.size(); ++j)
    {
        printf("%lu <-> %lu:", i, j);
        double d = a[i].distance(a[j]);
        double l = a[i].length() + a[j].length();
        printf("%f\n", d / l);
    }
    //while (cv::waitKey(0) != 'q');
    return 0;
}
