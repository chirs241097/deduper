//Chris Xiong 2022
//License: MPL-2.0
#include "signature.hpp"

#include <vector>
#include <filesystem>

namespace cv
{
    class Mat;
};

class subsliced_signature
{
public:
    signature full;
    std::vector<signature> subslices;
    size_t nhslices, nvslices;

    static subsliced_signature from_path(const std::filesystem::path &path,
                                         size_t nhslices, size_t nvslices,
                                         const signature_config &fcfg,
                                         const signature_config &scfg);
    static subsliced_signature from_file(const char *fn,
                                         size_t nhslices, size_t nvslices,
                                         const signature_config &fcfg,
                                         const signature_config &scfg);
    static subsliced_signature from_cvmatrix(cv::Mat *m,
                                             size_t nhslices, size_t nvslices,
                                             const signature_config &fcfg,
                                             const signature_config &scfg);
};
