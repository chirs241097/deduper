//Chris Xiong 2022
//License: MPL-2.0
/* Based on
 * H. Chi Wong, M. Bern and D. Goldberg,
 * "An image signature for any kind of image," Proceedings.
 * International Conference on Image Processing, 2002, pp. I-I,
 * doi: 10.1109/ICIP.2002.1038047.
 * and
 * libpuzzle (also an implementation of the article above).
 */
#include "signature.hpp"

#include <algorithm>
#include <fstream>
#include <cmath>
#include <cstdint>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "compressed_vector.hpp"
#include "imageutil.hpp"

static signature_config _default_cfg =
{
    9,     //slices
    3,     //blur_window
    2,     //min_window
    true,  //crop
    false, //comp
    0.5,   //pr
    1./128,//noise_threshold
    0.05,  //contrast_threshold
    0.25   //max_cropping
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
    signature_config cfg;
public:
    float get_light_charistics_cell(int x, int y, int w, int h);
    void get_light_charistics();
    void get_light_variance();
    void get_signature();
    double length() const;
    double distance(const signature_priv &o) const;
    bool operator==(const signature_priv &o) const;
    void dump() const;
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
    slc = cfg.slices;
    windowx = iw / (double)slc / 2;
    windowy = ih / (double)slc / 2;
    int windows = round(std::min(iw, ih) / slc * cfg.pr);
    if (windows < cfg.min_window)
        windows = cfg.min_window;
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
    int slc = cfg.slices;
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
        if (fabsf(l) > cfg.noise_threshold)
        {
            if (l > 0)
                lights.push_back(l);
            else
                darks.push_back(l);
        }
    }
    double lth = image_util::median(lights);
    double dth = image_util::median(darks);
    if (cfg.compress)
    {
        compressed = true;
        for (float &l : lv)
        {
            if (fabsf(l) > cfg.noise_threshold)
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
            if (fabsf(l) > cfg.noise_threshold)
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
        return image_util::distance(ct, o.ct) / (image_util::length(ct, uint8_t(2)) + image_util::length(o.ct, uint8_t(2)));
    else
        return image_util::distance(uct, o.uct) / (image_util::length(uct, uint8_t(2)) + image_util::length(o.uct, uint8_t(2)));
}

bool signature_priv::operator==(const signature_priv &o) const
{
    if (compressed && o.compressed)
        return ct == o.ct;
    else
        return uct == o.uct;
}

void signature_priv::dump() const
{
    if (!compressed)
        for (auto &x : this->uct)
            printf("%u ", x);
    else
        for (size_t i = 0; i < this->ct.size(); ++i)
            printf("%u ", this->ct.get(i));
    printf("\n");
}

signature::signature() = default;
signature::signature(signature_priv* _p) : p(_p){}
signature::~signature() = default;

void signature::dump() const
{
    if (p) p->dump();
}

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

signature signature::from_preprocessed_matrix(cv::Mat *m, const signature_config &cfg)
{
    signature_priv *p = new signature_priv;
    p->cfg = cfg;

    if (cfg.crop)
        p->fimg = image_util::crop(*m, cfg.contrast_threshold, cfg.max_cropping);
    else
        p->fimg = *m;
    if (cfg.blur_window > 1)
        cv::blur(p->fimg, p->fimg, cv::Size(cfg.blur_window, cfg.blur_window));
    p->get_light_charistics();
    p->get_light_variance();
    p->get_signature();
    p->fimg.release();
    p->lch.release();
    p->lv.clear();
    return signature(p);
}

signature signature::from_cvmatrix(cv::Mat *m, const signature_config &cfg)
{
    cv::Mat ma, bw;
    double sc = 1;
    switch (m->depth())
    {
        case CV_8U: sc = 1. / 255; break;
        case CV_16U: sc = 1. / 65535; break;
    }
    m->convertTo(ma, CV_32F, sc);
    if (m->channels() == 4)
        ma = image_util::blend_white(ma);
    if (ma.channels() == 3)
        cv::cvtColor(ma, bw, cv::COLOR_RGB2GRAY);
    else
        bw = ma;
    return signature::from_preprocessed_matrix(&bw, cfg);
}

signature signature::from_file(const char *fn, const signature_config &cfg)
{
    cv::Mat img = cv::imread(fn, cv::IMREAD_UNCHANGED);
    return signature::from_cvmatrix(&img, cfg);
}

signature signature::from_path(const std::filesystem::path &path, const signature_config &cfg)
{
    cv::Mat img = image_util::imread_path(path, cv::IMREAD_UNCHANGED);
    return signature::from_cvmatrix(&img, cfg);
}

signature_config signature::default_cfg()
{
    return _default_cfg;
}

size_t signature_hash::operator()(signature const& sig) const noexcept
{
    if (sig.p->compressed)
        return compressed_vector_hash<uint8_t, 3>{}(sig.p->ct);
    else
    {
        size_t ret = 0;
        for (uint8_t &v : sig.p->uct)
            ret ^= v + 0x9e3779b9 + (ret << 6) + (ret >> 2);
        return ret;
    }
}
