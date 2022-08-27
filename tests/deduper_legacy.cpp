#include "signature.hpp"

#include <cstdio>
#include <cstring>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <getopt.h>

#include "thread_pool.hpp"

int ctr;
int recursive;
int njobs=1;
double threshold=0.3;
std::vector<std::string> paths;

int parse_arguments(int argc,char **argv)
{
	recursive=0;
	int help=0;
	option longopt[]=
	{
		{"recursive",no_argument      ,&recursive,1},
//		{"destdir"  ,required_argument,0         ,'D'},
		{"jobs"     ,required_argument,0         ,'j'},
		{"threshold",required_argument,0         ,'d'},
		{"help"     ,no_argument      ,&help     ,1},
		{0          ,0                ,0         ,0}
	};
	while(1)
	{
		int idx=0;
		int c=getopt_long(argc,argv,"rhj:d:",longopt,&idx);
		if(!~c)break;
		switch(c)
		{
			case 0:
				if(longopt[idx].flag)break;
				if(std::string("jobs")==longopt[idx].name)
				sscanf(optarg,"%d",&njobs);
				if(std::string("threshold")==longopt[idx].name)
				sscanf(optarg,"%lf",&threshold);
			break;
			case 'r':
				recursive=1;
			break;
			case 'h':
				help=1;
			break;
			case 'j':
				sscanf(optarg,"%d",&njobs);
			break;
			case 'd':
				sscanf(optarg,"%lf",&threshold);
			break;
		}
	}
	for(;optind<argc;++optind)
		paths.push_back(argv[optind]);
	if(help||argc<2)
	{
		printf(
		"Usage: %s [OPTION] PATH...\n"
		"Detect potentially duplicate images in PATHs and optionally perform an action on them.\n\n"
		" -h, --help        Display this help message and exit.\n"
		" -r, --recursive   Recurse into all directories.\n"
		" -j, --jobs        Number of concurrent tasks to run at once.\n"
		" -d, --threshold   Threshold distance below which images will be considered similar.\n"
		,argv[0]
		);
		return 1;
	}
	if(threshold>1||threshold<0)
	{
		puts("Invalid threshold value.");
		return 2;
	}
	if(threshold<1e-6)threshold=1e-6;
	if(!paths.size())
	{
		puts("Missing image path.");
		return 2;
	}
	return 0;
}

void build_file_list(std::filesystem::path path,bool recursive,std::vector<std::string>&out)
{
	if(recursive)
	{
		auto dirit=std::filesystem::recursive_directory_iterator(path);
		for(auto &p:dirit)
		{
			FILE* fp=fopen(p.path().c_str(),"r");
			char c[8];
			fread((void*)c,1,6,fp);
			if(!memcmp(c,"\x89PNG\r\n",6)||!memcmp(c,"\xff\xd8\xff",3))
				out.push_back(p.path().string());
			fclose(fp);
		}
	}
	else
	{
		auto dirit=std::filesystem::directory_iterator(path);
		for(auto &p:dirit)
		{
			FILE* fp=fopen(p.path().c_str(),"r");
			char c[8];
			fread((void*)c,1,6,fp);
			if(!memcmp(c,"\x89PNG\r\n",6)||!memcmp(c,"\xff\xd8\xff",3))
				out.push_back(p.path().string());
			fclose(fp);
		}
	}
}

void compute_signature_vectors(const std::vector<std::string>&files,std::vector<signature>&output)
{
	thread_pool tp(njobs);
	for(size_t i=0;i<files.size();++i)
	{
		auto job_func=[&](int thid,size_t id){
			fprintf(stderr,"spawned: on thread#%d, file#%lu (%s)\n",thid,id,files[id].c_str());
                        output[id]=signature::from_file(files[id].c_str());
			fprintf(stderr,"done: file#%lu\n",id);
                        output[id].length();
			printf("%d/%lu\r",++ctr,files.size());
			fflush(stdout);
		};
		tp.create_task(job_func,i);
	}
	tp.wait();
}

void compare_signature_vectors(const std::vector<signature>&vec,std::vector<std::tuple<size_t,size_t,double>>&out)
{
	thread_pool tp(njobs);
	for(size_t i=0;i<vec.size();++i){if (vec[i].length() < 0) continue;
	for(size_t j=i+1;j<vec.size();++j)
	{
                if (vec[j].length() < 0) continue;
		auto job_func=[&](int thid,size_t ida,size_t idb){
			fprintf(stderr,"spawned: on thread#%d, file#%lu<->file#%lu\n",thid,ida,idb);
			if(true)
			{
				double d=vec[ida].distance(vec[idb]);
                                double l=vec[ida].length()+vec[idb].length();
                                d/=l;
				if(d<threshold)out.emplace_back(ida,idb,d);
				fprintf(stderr,"done:file#%lu<->file#%lu: %lf\n",ida,idb,d);
			}
			printf("%d/%lu\r",++ctr,vec.size()*(vec.size()-1)/2);
			fflush(stdout);
		};
		tp.create_task(job_func,i,j);
	}}
	tp.wait();
}

int main(int argc,char** argv)
{
	if(int pr=parse_arguments(argc,argv))return pr-1;
	puts("building list of files to compare...");
	std::vector<std::string> x;
	for(auto&p:paths)
		build_file_list(p,recursive,x);
	printf("%lu files to compare.\n",x.size());
	puts("computing signature vectors...");
	std::vector<signature> cvecs;
	cvecs.resize(x.size());
	compute_signature_vectors(x,cvecs);
	/*for(auto &v:cvecs)
	{
		fprintf(stderr,"%lu:",v.sizeof_vec);
		for(size_t i=0;i<v.sizeof_vec;++i)
		fprintf(stderr," %d",v.vec[i]);
		fprintf(stderr,"\n");
	}*/
	ctr=0;
	puts("\ncomparing signature vectors...");
	std::vector<std::tuple<size_t,size_t,double>> r;
	compare_signature_vectors(cvecs,r);
	puts("");
	for(auto &t:r)
		printf("%s<->%s: %lf\n",x[std::get<0>(t)].c_str(),x[std::get<1>(t)].c_str(),std::get<2>(t));
	printf("%lu similar images.",r.size());
	cvecs.clear();
	return 0;
}
