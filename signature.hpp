#ifndef SIGNATURE_HPP
#define SIGNATURE_HPP

#include <memory>
#include <opencv2/core.hpp>

struct signature_config
{
    int slices;
    int blur_window;
    int min_window;
    bool crop;
    bool compress;
    double pr;
    double noise_threshold;
    double contrast_threshold;
    double max_cropping;
};

class signature_priv;
class signature
{
private:
    std::shared_ptr<signature_priv> p;
    static signature_config cfg;
    signature(signature_priv* _p);
    signature(const signature&)=default;
    signature& operator=(const signature&)=default;
public:
    signature();
    ~signature();
    signature(signature&&)=default;
    signature& operator=(signature&&)=default;
    signature clone() const;//do not use unless absolutely needed
    double length() const;
    double distance(const signature &o) const;
    bool operator ==(const signature &o) const;

    /*
     * Configure parameters for signature calculation.
     * Please note:
     * Comparing signatures calculated using different
     * parameters gives no meaningful results.
     *
     * If never called, a default configuration is used.
     * See signature.cpp.
     */
    static void configure(signature_config _cfg);
    /*
     * Get current signature calculation parameters.
     * If it's never set explicitly, the default configuration
     * is returned.
     */
    static signature_config config();
    static signature from_file(const char *fn);

    /*
     * Input will be stripped of alpha channel (by blending with white),
     * converted to single channel (rgb2gray).
     * Then it will be passed to from_preprocessed_matrix.
     * The matrix doesn't have to be continuous.
     */
    static signature from_cvmatrix(cv::Mat m);

    /*
     * Input must be a single channel, floating point matrix
     * (values clamped to 0-1)
     * The matrix must be continuous if cropping is used
     * STILL *Will* be cropped if config().crop == true
     * STILL *Will* be blurred if config().blur_window > 1
     */
    static signature from_preprocessed_matrix(cv::Mat m);

    friend class signature_priv;
    friend struct signature_hash;
};

struct signature_hash
{
    size_t operator()(signature const& sig) const noexcept;
};

#endif
