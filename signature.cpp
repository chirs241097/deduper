/* Based on
 * H. Chi Wong, M. Bern and D. Goldberg,
 * "An image signature for any kind of image," Proceedings.
 * International Conference on Image Processing, 2002, pp. I-I,
 * doi: 10.1109/ICIP.2002.1038047.
 * and
 * libpuzzle (which is also based on the article above).
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "compressed_vector.hpp"
#include "imageutil.hpp"
#include "signature.hpp"

signature_config signature::cfg =
{
    9,
    3,
    2,
    true,
    false,
    0.5,
    1./128,
    0.05,
    0.25
};

class signature_priv
{
private:
    cv::Mat fimg;
    cv::Mat lch;
    std::vector<float> lv;
    compressed_vector<uint8_t, 3> ct;
    std::vector<uint8_t> uct;
    bool compressed;
public:
    float get_light_charistics_cell(int x, int y, int w, int h);
    void get_light_charistics();
    void get_light_variance();
    void get_signature();
    double length() const;
    double distance(const signature_priv &o) const;
    bool operator==(const signature_priv &o) const;
    friend class signature;
    friend struct signature_hash;
};

float signature_priv::get_light_charistics_cell(int x, int y, int w, int h)
{
    return cv::mean(fimg(cv::Range(y, y + h), cv::Range(x, x + w)))[0];
}

void signature_priv::get_light_charistics()
{
    double windowx, windowy;
    int iw, ih, slc;
    iw = fimg.size().width;
    ih = fimg.size().height;
    slc = signature::cfg.slices;
    windowx = iw / (double)slc / 2;
    windowy = ih / (double)slc / 2;
    int windows = round(std::min(iw, ih) / slc * signature::cfg.pr);
    if (windows < signature::cfg.min_window)
        windows = signature::cfg.min_window;
    double ww = (iw - 1) / (slc + 1.);
    double wh = (ih - 1) / (slc + 1.);
    double wxs = 0, wys = 0;
    if (windows < ww) wxs = (ww - windows) / 2.;
    if (windows < wh) wys = (wh - windows) / 2.;
    lch.create(slc, slc, CV_32F);
    float *lp = lch.ptr<float>(0);
    for (int i = 0; i < slc; ++i)
    {
        for (int j = 0; j < slc; ++j)
        {
            double cwx, cwy;
            cwx = i * (iw - 1) / (slc + 1.) + windowx;
            cwy = j * (ih - 1) / (slc + 1.) + windowy;
            int x = (int)round(cwx + wxs);
            int y = (int)round(cwy + wys);
            int cww, cwh;
            cww = (iw - x < windows) ? 1 : windows;
            cwh = (ih - y < windows) ? 1 : windows;
            *(lp++) = get_light_charistics_cell(x, y, cww, cwh);
        }
    }
}

void signature_priv::get_light_variance()
{
    const int dx[8] = {-1, -1, -1,  0,  0,  1,  1,  1};
    const int dy[8] = {-1,  0,  1, -1,  1, -1,  0,  1};
    int slc = signature::cfg.slices;
    float *lp = lch.ptr<float>(0);
    for (int x = 0; x < slc; ++x)
    {
        for (int y = 0; y < slc; ++y)
        {
            for (int k = 0; k < 8; ++k)
            {
                int nx = x + dx[k];
                int ny = y + dy[k];
                if (nx < 0 || ny < 0 || nx >= slc || ny >= slc)
                    lv.push_back(0);
                else
                    lv.push_back(*lp - *(lp + dx[k] * slc + dy[k]));
            }
            ++lp;
        }
    }
}

void signature_priv::get_signature()
{
    std::vector<double> lights;
    std::vector<double> darks;
    for (float &l : lv)
    {
        if (fabsf(l) > signature::cfg.noise_threshold)
        {
            if (l > 0)
                lights.push_back(l);
            else
                darks.push_back(l);
        }
    }
    double lth = image_util::median(lights);
    double dth = image_util::median(darks);
    if (signature::cfg.compress)
    {
        compressed = true;
        for (float &l : lv)
        {
            if (fabsf(l) > signature::cfg.noise_threshold)
            {
                if (l > 0)
                    ct.push_back(l > lth ? 4 : 3);
                else
                    ct.push_back(l < dth ? 0 : 1);
            }
            else ct.push_back(2);
        }
    }
    else
    {
        compressed = false;
        for (float &l : lv)
        {
            if (fabsf(l) > signature::cfg.noise_threshold)
            {
                if (l > 0)
                    uct.push_back(l > lth ? 4 : 3);
                else
                    uct.push_back(l < dth ? 0 : 1);
            }
            else uct.push_back(2);
        }
    }
}

double signature_priv::length() const
{
    if (compressed)
        return image_util::length(ct, (uint8_t)2);
    else
        return image_util::length(uct, 2);
}

double signature_priv::distance(const signature_priv &o) const
{
    if (compressed && o.compressed)
        return image_util::distance(ct, o.ct);
    else
        return image_util::distance(uct, o.uct);
}

bool signature_priv::operator==(const signature_priv &o) const
{
    if (compressed && o.compressed)
        return ct == o.ct;
    else
        return uct == o.uct;
}

signature::signature() = default;
signature::signature(signature_priv* _p) : p(_p) {}
signature::~signature() = default;

signature signature::clone() const
{
    return signature(*this);
}

double signature::length() const
{
    if (!p) {fprintf(stderr, "length: null signature"); return -1;}
    return p->length();
}

double signature::distance(const signature &o) const
{
    if (!p || !o.p) {fprintf(stderr, "distance: null signature"); return -1;}
    return p->distance(*o.p);
}

bool signature::operator==(const signature &o) const
{
    if (!p || !o.p) {fprintf(stderr, "eq: null signature"); return false;}
    return *p == *o.p;
}

void signature::configure(signature_config _cfg)
{signature::cfg = _cfg;}

signature_config signature::config()
{return signature::cfg;}

signature signature::from_preprocessed_matrix(cv::Mat m)
{
    signature_priv *p = new signature_priv;
    if (signature::cfg.crop)
        p->fimg = image_util::crop(m, signature::cfg.contrast_threshold, signature::cfg.max_cropping);
    else
        p->fimg = m;
    if (signature::cfg.blur_window > 1)
        cv::blur(p->fimg, p->fimg, cv::Size(signature::cfg.blur_window, signature::cfg.blur_window));
    p->get_light_charistics();
    p->get_light_variance();
    p->get_signature();
    p->fimg.release();
    p->lch.release();
    p->lv.clear();
    return signature(p);
}

signature signature::from_cvmatrix(cv::Mat m)
{
    cv::Mat ma, bw;
    double sc = 1;
    switch (m.depth())
    {
        case CV_8U: sc = 1. / 255; break;
        case CV_16U: sc = 1. / 65535; break;
    }
    m.convertTo(ma, CV_32F, sc);
    if (m.channels() == 4)
        ma = image_util::blend_white(ma);
    if (ma.channels() == 3)
        cv::cvtColor(ma, bw, cv::COLOR_RGB2GRAY);
    else
        bw = ma;
    return signature::from_preprocessed_matrix(bw);
}

signature signature::from_file(const char *fn)
{
    cv::Mat img = cv::imread(fn, cv::IMREAD_UNCHANGED);
    return signature::from_cvmatrix(img);
}

size_t signature_hash::operator()(signature const& sig) const noexcept
{
    return compressed_vector_hash<uint8_t, 3>{}(sig.p->ct);
}
