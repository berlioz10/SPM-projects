#include <omp.h>  // used here just for omp_get_wtime()
#include <cstring>
#include <vector>
#include <set>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>

#pragma omp declare reduction(merge : std::vector<std::string> : \
	omp_out.insert(omp_out.end(), omp_in.begin(), omp_in.end())) 

#pragma omp declare reduction(merge : std::vector<std::pair<std::string, uint64_t>> : \
	omp_out.insert(omp_out.end(), omp_in.begin(), omp_in.end())) 

using umap=std::unordered_map<std::string, uint64_t>;
using pair=std::pair<std::string, uint64_t>;
struct Comp {
	bool operator ()(const pair& p1, const pair& p2) const {
		return p1.second > p2.second;
	}
};
using ranking=std::multiset<pair, Comp>;

// ------ globals --------
uint64_t total_words{0};
volatile uint64_t extraworkXline{0};
// ----------------------

void tokenize_line(const std::string& line, std::vector<pair>& pairs) {
	char *tmpstr;
	umap UM;
	char *token = strtok_r(const_cast<char*>(line.c_str()), " \r\n", &tmpstr);
	while(token) {
		++UM[std::string(token)];
		token = strtok_r(NULL, " \r\n", &tmpstr);
		++total_words;
	}
	pairs.insert(pairs.end(), UM.begin(), UM.end());
	for(volatile uint64_t j{0}; j<extraworkXline; j++);
}

void compute_file(const std::string& filename, std::vector<std::string>& V) {
	{

		std::ifstream file(filename, std::ios_base::in);
		if (file.is_open()) {
			std::string line;
				while(std::getline(file, line)) {
					if (!line.empty()) {
						V.push_back(line);
					}
				}
			}
		file.close();
	}
}

void reduce_maps(umap& map1, umap& map2) {
	for(auto p : map2) {
		map1[p.first] += p.second;
	}
}

int main(int argc, char *argv[]) {

	auto usage_and_exit = [argv]() {
		std::printf("use: %s filelist.txt [extraworkXline] [topk] [showresults]\n", argv[0]);
		std::printf("     filelist.txt contains one txt filename per line\n");
		std::printf("     extraworkXline is the extra work done for each line, it is an integer value whose default is 0\n");
		std::printf("     topk is an integer number, its default value is 10 (top 10 words)\n");
		std::printf("     showresults is 0 or 1, if 1 the output is shown on the standard output\n\n");
		exit(-1);
	};

	std::vector<std::string> filenames;
	size_t topk = 10;
	bool showresults=false;
	if (argc < 2 || argc > 5) {
		usage_and_exit();
	}

	if (argc > 2) {
		try { extraworkXline=std::stoul(argv[2]);
		} catch(std::invalid_argument const& ex) {
			std::printf("%s is an invalid number (%s)\n", argv[2], ex.what());
			return -1;
		}
		if (argc > 3) {
			try { topk=std::stoul(argv[3]);
			} catch(std::invalid_argument const& ex) {
				std::printf("%s is an invalid number (%s)\n", argv[3], ex.what());
				return -1;
			}
			if (topk==0) {
				std::printf("%s must be a positive integer\n", argv[3]);
				return -1;
			}
			if (argc == 5) {
				int tmp;
				try { tmp=std::stol(argv[4]);
				} catch(std::invalid_argument const& ex) {
					std::printf("%s is an invalid number (%s)\n", argv[4], ex.what());
					return -1;
				}
				if (tmp == 1) showresults = true;
			}
		}
	}
	
	if (std::filesystem::is_regular_file(argv[1])) {
		std::ifstream file(argv[1], std::ios_base::in);
		if (file.is_open()) {
			std::string line;
			while(std::getline(file, line)) {
				if (std::filesystem::is_regular_file(line))
					filenames.push_back(line);
				else
					std::cout << line << " is not a regular file, skipt it\n";
			}					
		} else {
			std::printf("ERROR: opening file %s\n", argv[1]);
			return -1;
		}
		file.close();
	} else {
		std::printf("%s is not a regular file\n", argv[1]);
		usage_and_exit();
	}

	// omp_set_num_threads(8);
	int num_threads = omp_get_max_threads();
	std::printf("Number of threads is: %d\n", num_threads);
	// used for storing results
	std::vector<umap> vUM;
	std::vector<std::string> V;
	std::vector<pair> pairs;
	for(int i = 0; i < num_threads; i ++) {
		vUM.push_back(umap());
	}
	umap UM;

	auto start = omp_get_wtime();	
	
	#pragma omp parallel for reduction(merge : V)
	for (int i = 0; i < filenames.size(); i++) {
		compute_file(filenames[i], V);
	}
	
	#pragma omp parallel shared(vUM, UM, pairs)
	{
		#pragma omp for reduction(merge : pairs)
		for (int i = 0; i < V.size(); i++) {
			tokenize_line(V[i], pairs);
		}
		#pragma omp barrier

		#pragma omp for nowait
		for(int i = 0; i < pairs.size(); i++) {
			// local map for each of them
			vUM[omp_get_thread_num()][pairs[i].first] += pairs[i].second;
		}

		#pragma omp critical
		{
			reduce_maps(UM, vUM[omp_get_thread_num()]);
		}
	}

	auto stop1 = omp_get_wtime();
	
	// sorting in descending order
	ranking rank(UM.begin(), UM.end());

	auto stop2 = omp_get_wtime();
	std::printf("Compute time (s) %f\nSorting time (s) %f\n",
				stop1 - start, stop2 - stop1);
	
	if (showresults) {
		// show the results
		std::cout << "Unique words " << rank.size() << "\n";
		std::cout << "Total words  " << total_words << "\n";
		std::cout << "Top " << topk << " words:\n";
		auto top = rank.begin();
		for (size_t i=0; i < std::clamp(topk, 1ul, rank.size()); ++i)
			std::cout << top->first << '\t' << top++->second << '\n';
	}
}
	
