#include <cstdio>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include <QWidget>
#include <QApplication>
#include "mingui.hpp"

using std::size_t;

std::unordered_map<std::string, size_t> p;
std::vector<std::string> fns;
std::map<std::pair<size_t, size_t>, double> dist;
std::vector<size_t> par;
std::vector<std::vector<size_t>> lists;

MinGuiWidget *w = nullptr;
size_t curlist;

size_t get_root(size_t x)
{
    if (x != par[x])
        return par[x] = get_root(par[x]);
    return x;
}

void combine(size_t x, size_t y)
{
    x = get_root(x);
    y = get_root(y);
    par[x] = y;
}

void load_result(const char* rp)
{
    FILE *f = fopen(rp, "rb");
    while (1)
    {
        int l;
        double d;
        std::string s1, s2;
        if (feof(f)) break;
        fread(&l, sizeof(int), 1, f);
        s1.resize(l);
        fread(s1.data(), 1, l, f);
        p.try_emplace(s1, p.size() + 1);
        fread(&l, sizeof(int), 1, f);
        s2.resize(l);
        fread(s2.data(), 1, l, f);
        p.try_emplace(s2, p.size() + 1);
        fread(&d, sizeof(double), 1, f);
        dist[std::make_pair(p[s1], p[s2])] = d;
    }
    fclose(f);
}

std::vector<std::string> build_list(const std::vector<size_t> &l)
{
    std::vector<std::string> ret;
    for (auto &x : l)
        ret.push_back(fns[x]);
    return ret;
}

std::map<std::pair<size_t, size_t>, double> build_dists(const std::vector<size_t> &l)
{
    std::map<std::pair<size_t, size_t>, double> ret;
    for (size_t i = 0; i < l.size(); ++i)
    {
        for (size_t j = i + 1; j < l.size(); ++j)
        {
            size_t x = l[i], y = l[j];
            if (dist.find(std::make_pair(x, y)) != dist.end())
                ret[std::make_pair(i, j)] = dist[std::make_pair(x, y)];
            else if (dist.find(std::make_pair(y, x)) != dist.end())
                ret[std::make_pair(i, j)] = dist[std::make_pair(y, x)];
        }
    }
    return ret;
}

int main(int argc, char **argv)
{
    if (argc < 2) return 1;

    load_result(argv[1]);
    printf("%lu known files\n", p.size());

    par.resize(p.size() + 1);
    fns.resize(p.size() + 1);
    lists.resize(p.size() + 1);
    for (auto &kp : p)
        fns[kp.second] = kp.first;

    for (size_t i = 1; i < par.size(); ++i)
        par[i] = i;
    for (auto &kp : dist)
    {
        auto p = kp.first;
        combine(p.first, p.second);
    }
    for (size_t i = 1; i < par.size(); ++i)
        lists[get_root(i)].push_back(i);

    auto listend = std::remove_if(lists.begin(), lists.end(), [](auto &a){return a.size() < 2;});
    lists.erase(listend, lists.end());
    if (lists.empty()) return 0;
    for (auto &l : lists)
    {
        if (l.size())
        {
            for (auto &x : l)
                printf("%s,", fns[x].c_str());
            puts("");
        }
    }
    fflush(stdout);

    QApplication a(argc, argv);

    curlist = 0;
    w = new MinGuiWidget();
    w->show_images(build_list(lists[curlist]));
    w->update_distances(build_dists(lists[curlist]));
    w->update_permamsg(curlist, lists.size());
    w->show();
    QObject::connect(w, &MinGuiWidget::next,
                     []{
                           if (curlist < lists.size() - 1) ++curlist;
                           w->show_images(build_list(lists[curlist]));
                           w->update_distances(build_dists(lists[curlist]));
                           w->update_permamsg(curlist, lists.size());

    });
    QObject::connect(w, &MinGuiWidget::prev,
                     []{
                           if (curlist > 0) --curlist;
                           w->show_images(build_list(lists[curlist]));
                           w->update_distances(build_dists(lists[curlist]));
                           w->update_permamsg(curlist, lists.size());
    });

    a.exec();

    return 0;
}
