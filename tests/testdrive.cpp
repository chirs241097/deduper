#include <cstdio>
#include <cstring>

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <thread>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <getopt.h>

#ifdef _WIN32 //for the superior operating system
#include <cwchar>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processenv.h>
#include <shellapi.h>
#endif

#include "signature.hpp"
#include "imageutil.hpp"

#include "thread_pool.hpp"

#define DEBUG 0

namespace fs = std::filesystem;

int ctr;
int recursive;
int njobs = 1;
double threshold = 0.3;
std::vector<fs::path> paths;
std::vector<fs::path> files;

int nsliceh = 3;
int nslicev = 3;

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

struct sig_eq
{
    bool operator()(const signature& a, const signature& b) const
    {
        //return a.distance(b) < 0.1;
        return a == b;
    }
};

typedef std::pair<size_t, int> slice_info;
std::unordered_map<signature, std::vector<slice_info>, signature_hash, sig_eq> slices;
std::vector<signature> signatures;
std::mutex sigmtx;
std::vector<std::pair<size_t, size_t>> out;

int parse_arguments(int argc,char **argv)
{
    recursive = 0;
    int help = 0;
    option longopt[]=
    {
        {"recursive", no_argument      , &recursive, 1},
//      {"destdir"  , required_argument, 0         , 'D'},
        {"jobs"     , required_argument, 0         , 'j'},
//      {"threshold", required_argument, 0         , 'd'},
        {"help"     , no_argument      , &help     , 1},
        {0          , 0                , 0         , 0}
    };
    while(1)
    {
        int idx = 0;
        int c = getopt_long(argc, argv, "rhj:", longopt, &idx);
        if (!~c) break;
        switch (c)
        {
            case 0:
                if (longopt[idx].flag) break;
                if (std::string("jobs") == longopt[idx].name)
                    sscanf(optarg, "%d", &njobs);
                //if(std::string("threshold") == longopt[idx].name)
                    //sscanf(optarg, "%lf", &threshold);
            break;
            case 'r':
                recursive = 1;
            break;
            case 'h':
                help = 1;
            break;
            case 'j':
                sscanf(optarg, "%d", &njobs);
            break;
            case 'd':
                //sscanf(optarg, "%lf", &threshold);
            break;
        }
    }
#ifdef _WIN32 //w*ndows, ugh
    wchar_t *args = GetCommandLineW();
    int wargc;
    wchar_t **wargv = CommandLineToArgvW(args, &wargc);
    if (wargv && wargc == argc)
    {
        for (; optind < argc; ++optind)
            paths.push_back(wargv[optind]);
    }
#else
    for (; optind < argc; ++optind)
        paths.push_back(argv[optind]);
#endif
    if (help || argc < 2)
    {
        printf(
        "Usage: %s [OPTION] PATH...\n"
        "Detect potentially duplicate images in PATHs and optionally perform an action on them.\n\n"
        " -h, --help        Display this help message and exit.\n"
        " -r, --recursive   Recurse into all directories.\n"
        " -j, --jobs        Number of concurrent tasks to run at once.\n"
//      " -d, --threshold   Threshold distance below which images will be considered similar.\n"
        ,argv[0]
        );
        return 1;
    }
    if (threshold > 1 || threshold < 0)
    {
        puts("Invalid threshold value.");
        return 2;
    }
    if (threshold < 1e-6) threshold = 1e-6;
    if (!paths.size())
    {
        puts("Missing image path.");
        return 2;
    }
    return 0;
}

void build_file_list(fs::path path, bool recursive, std::vector<fs::path> &out)
{
    if (recursive)
    {
        auto dirit = fs::recursive_directory_iterator(path);
        for (auto &p : dirit)
        {
            std::fstream st(p.path(), std::ios::binary | std::ios::in);
            char c[8];
            st.read(c, 6);
            if (st.gcount() < 6) continue;
            if(!memcmp(c,"\x89PNG\r\n", 6) || !memcmp(c,"\xff\xd8\xff", 3))
            {
                out.push_back(p.path().string());
#if DEBUG > 0
                printf("%ld, %s\n", out.size() - 1, out.back().c_str());
#endif
            }
            st.close();
        }
    }
    else
    {
        auto dirit = fs::directory_iterator(path);
        for(auto &p : dirit)
        {
            std::fstream st(p.path(), std::ios::binary | std::ios::in);
            char c[8];
            st.read(c, 6);
            if (st.gcount() < 6) continue;
            if(!memcmp(c,"\x89PNG\r\n", 6) || !memcmp(c,"\xff\xd8\xff", 3))
            {
                out.push_back(p.path().string());
#if DEBUG > 0
                printf("%ld, %s\n", out.size() - 1, out.back().c_str());
#endif
            }
            st.close();
        }
    }
}

void job_func(int thid, size_t id)
{
    cv::Mat img = image_util::imread_path(files[id], cv::IMREAD_UNCHANGED);
    signature s = signature::from_cvmatrix(&img, cfg_full);
#if DEBUG > 1
    s.dump();
#endif
    int ssw = img.size().width / nsliceh;
    int ssh = img.size().height / nslicev;
    std::vector<signature> subsigs;
    for (int i = 0; i < nsliceh; ++i)
    for (int j = 0; j < nslicev; ++j)
    {
        int l = i * ssw;
        int r = (i == nsliceh) ? img.size().width : (i + 1) * ssw;
        int t = j * ssh;
        int b = (j == nslicev) ? img.size().height : (j + 1) * ssh;
        cv::Mat slice = img(cv::Range(t, b), cv::Range(l, r));
        subsigs.push_back(std::move(signature::from_cvmatrix(&slice, cfg_subslice)));
#if DEBUG > 0
        printf("%ld, (%d, %d) %lu\n", id, i, j, signature_hash{}(subsigs.back()));
#endif
#if DEBUG > 1
        subsigs.back().dump();
#endif
    }

    printf("%d %lu\r", thid, id);
    fflush(stdout);

    sigmtx.lock();
    std::vector<bool> v;
    v.resize(files.size());
    for (int i = 0; i < nsliceh * nslicev; ++i)
    {
        auto it = slices.find(subsigs[i]);
        if (it != slices.end())
        {
            for (auto &si : it->second)
            {
                if (si.second == i)
                {
#if DEBUG > 1
                    printf("%d@(%ld <-> %ld) %f\n", i, id, si.first, s.distance(signatures[si.first]));
#endif

                    if (!v[si.first] && s.distance(signatures[si.first]) < threshold)
                    {
                        out.emplace_back(id, std::move(si.first));
                    }
                    v[si.first] = true;
                }
            }
            it->second.emplace_back(id, i);
        }
        else
        {
            slices.emplace(std::move(subsigs[i].clone()),
                           std::vector<slice_info>{{id, i}});
        }
    }
    signatures[id] = std::move(s);
    sigmtx.unlock();
}

void run()
{
    thread_pool tp(njobs);
    for(size_t i = 0; i < files.size(); ++i)
    {
        tp.create_task(job_func, i);
    }
    tp.wait();
}

int main(int argc,char** argv)
{
    if (int pr = parse_arguments(argc, argv)) return pr - 1;
    puts("building list of files to compare...");
    for (auto &p : paths)
        build_file_list(p, recursive, files);
    printf("%lu files to compare.\n", files.size());
    puts("computing signature vectors...");

    signatures.resize(files.size());
    run();
    FILE *outf = fopen("result", "wb");
    for (auto &p : out)
    {
#ifdef _WIN32
        wprintf(L"%ls %ls %f\n", files[p.first].c_str(), files[p.second].c_str(), signatures[p.first].distance(signatures[p.second]));
#else
        printf("%s %s %f\n", files[p.first].c_str(), files[p.second].c_str(), signatures[p.first].distance(signatures[p.second]));
#endif
        int t;
        double ts;
        t = (int)files[p.first].native().length();
        fwrite(&t, sizeof(int), 1, outf);
        fwrite(files[p.first].c_str(), sizeof(fs::path::value_type), t, outf);
        t = (int)files[p.second].native().length();
        fwrite(&t, sizeof(int), 1, outf);
        fwrite(files[p.second].c_str(), sizeof(fs::path::value_type), t, outf);
        ts = signatures[p.first].distance(signatures[p.second]);
        fwrite(&ts, sizeof(double), 1, outf);
    }
    fclose(outf);
    return 0;
}

