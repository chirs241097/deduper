#include <cstdio>
#include <cstring>

#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <thread>

#include <getopt.h>

#if PATH_VALSIZE == 2
#include <cwchar>
#endif

#ifdef _WIN32 //for the superior operating system
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processenv.h>
#include <shellapi.h>
#endif

#include "signature.hpp"
#include "subslice_signature.hpp"
#include "signature_db.hpp"

#include "thread_pool.hpp"

#define DEBUG 0

namespace fs = std::filesystem;

int ctr;
int recursive;
int njobs = 1;
double threshold = 0.3;
std::vector<fs::path> paths;
std::vector<fs::path> files;

size_t nsliceh = 3;
size_t nslicev = 3;

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

signature_db *sdb;

int parse_arguments(int argc, char **argv)
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
                out.push_back(p.path());
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
                out.push_back(p.path());
#if DEBUG > 0
                printf("%ld, %s\n", out.size() - 1, out.back().c_str());
#endif
            }
            st.close();
        }
    }
}

int main(int argc,char** argv)
{
    if (int pr = parse_arguments(argc, argv)) return pr - 1;
    puts("building list of files to compare...");
    for (auto &p : paths)
        build_file_list(p, recursive, files);
    printf("%lu files to compare.\n", files.size());
    puts("initializing database...");
    sdb = new signature_db();
    puts("computing signature vectors...");

    populate_cfg_t pcfg = {
        nsliceh,
        nslicev,
        cfg_full,
        cfg_subslice,
        threshold,
        [](size_t c, int){printf("%lu\r", c); fflush(stdout);},
        njobs
    };
    sdb->populate(files, pcfg);

    puts("grouping similar images...");
    sdb->group_similar();

    std::vector<dupe_t> dupes = sdb->dupe_pairs();
    for (auto &p : dupes)
    {
        fs::path p1, p2;
        std::tie(p1, std::ignore) = sdb->get_signature(p.id1);
        std::tie(p2, std::ignore) = sdb->get_signature(p.id2);
#if PATH_VALSIZE == 2
        wprintf(L"%ls %ls %f\n", p1.c_str(), p2.c_str(), p.distance);
#else
        printf("%s %s %f\n", p1.c_str(), p2.c_str(), p.distance);
#endif
    }

    std::vector<std::vector<size_t>> gp = sdb->groups_get();
    for (auto gi = gp.begin(); gi != gp.end(); ++gi)
    {
        if (gi->size() < 2) continue;
        printf("group #%lu:\n", gi - gp.begin());
        for (auto &id : *gi)
        {
            fs::path p;
            std::tie(p, std::ignore) = sdb->get_signature(id);
#if PATH_VALSIZE == 2
            wprintf(L"\t%ls\n", p.c_str());
#else
            printf("\t%s\n", p.c_str());
#endif
        }
    }

    sdb->to_db_file("test.sigdb");
    delete sdb;
    return 0;
}
