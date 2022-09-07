//Chris Xiong 2022
//License: MPL-2.0
#include "subslice_signature.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "imageutil.hpp"

subsliced_signature subsliced_signature::from_path(const std::filesystem::path &path,
                                                   size_t nhslices, size_t nvslices,
                                                   const signature_config &fcfg,
                                                   const signature_config &scfg)
{
    cv::Mat img = image_util::imread_path(path, cv::IMREAD_UNCHANGED);
    return subsliced_signature::from_cvmatrix(&img, nhslices, nvslices, fcfg, scfg);
}

subsliced_signature subsliced_signature::from_file(const char *fn,
                                                   size_t nhslices, size_t nvslices,
                                                   const signature_config &fcfg,
                                                   const signature_config &scfg)
{
    cv::Mat img = cv::imread(fn, cv::IMREAD_UNCHANGED);
    return subsliced_signature::from_cvmatrix(&img, nhslices, nvslices, fcfg, scfg);
}

subsliced_signature subsliced_signature::from_cvmatrix(cv::Mat *m,
                                             size_t nhslices, size_t nvslices,
                                             const signature_config &fcfg,
                                             const signature_config &scfg)
{
    subsliced_signature ret;
    ret.full = signature::from_cvmatrix(m, fcfg);
    cv::Mat *sm = m;
    if (m->size().width / nhslices > 100 || m->size().height / nvslices > 100)
    {
        double sc = 100. * nhslices / m->size().width;
        if (100. * nvslices / m->size().height < sc)
            sc = 100. * nvslices / m->size().height;
        sm = new cv::Mat();
        cv::resize(*m, *sm, cv::Size(), sc, sc, cv::InterpolationFlags::INTER_LINEAR);
    }
    ret.nhslices = nhslices;
    ret.nvslices = nvslices;
    int ssw = sm->size().width / nhslices;
    int ssh = sm->size().height / nvslices;
    for (int i = 0; i < nhslices; ++i)
    for (int j = 0; j < nvslices; ++j)
    {
        int l = i * ssw;
        int r = (i == nhslices) ? sm->size().width : (i + 1) * ssw;
        int t = j * ssh;
        int b = (j == nvslices) ? sm->size().height : (j + 1) * ssh;
        cv::Mat slice = (*sm)(cv::Range(t, b), cv::Range(l, r));
        ret.subslices.push_back(std::move(signature::from_cvmatrix(&slice, scfg)));
    }
    if (sm != m)
        delete sm;
    return ret;
}
