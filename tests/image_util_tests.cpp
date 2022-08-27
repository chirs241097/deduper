#include "imageutil.hpp"

#include <cstdio>
#include <string>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("usage: %s <image file>\n", argv[0]);
        return 1;
    }
    cv::Mat i = cv::imread(argv[1], cv::IMREAD_UNCHANGED);
    if (i.data == NULL)
    {
        printf("invalid image.\n");
        return 1;
    }
    cv::Mat fi, bw;
    double sc = 1;
    switch (i.depth())
    {
        case CV_8U: sc = 1. / 255; break;
        case CV_16U: sc = 1. / 65535; break;
    }
    i.convertTo(fi, CV_32F, sc);
    if (fi.channels() == 4)
        fi = image_util::blend_white(fi);
    cv::cvtColor(fi, bw, cv::COLOR_RGB2GRAY);
    cv::imshow(std::string("test"), bw);
    cv::Range xr, yr;
    double contrast_threshold = 0.05;
    double max_crop_ratio = 0.25;
    xr = image_util::crop_axis(bw, 0, contrast_threshold, max_crop_ratio);
    yr = image_util::crop_axis(bw, 1, contrast_threshold, max_crop_ratio);
    cv::Mat cfi = image_util::crop(bw, contrast_threshold, max_crop_ratio);
    cv::imshow(std::string("cropped"), cfi);
    printf("xxx [%d, %d) [%d, %d)\n", yr.start, yr.end, xr.start, xr.end);
    puts("press q to quit.");
    while (cv::waitKey(0) != 'q');
    return 0;
}
