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
#include <ff/ff.hpp>
#include <ff/map.hpp>
#include <random>

using namespace ff;

using umap=std::unordered_map<std::string, uint64_t>;
using pair=std::pair<std::string, uint64_t>;
using first_task=std::string;
using vector_string=std::vector<std::string>;
struct Comp {
	bool operator ()(const pair& p1, const pair& p2) const {
		return p1.second > p2.second;
	}
};
using ranking=std::multiset<pair, Comp>;

// ------ globals --------
std::atomic<uint32_t> total_words{0};
volatile uint64_t extraworkXline{0};
// ----------------------

void tokenize_line(const std::string& line, umap& UM) {
	char *tmpstr;
	char *token = strtok_r(const_cast<char*>(line.c_str()), " \r\n", &tmpstr);
	while(token) {
		++UM[std::string(token)];
		token = strtok_r(NULL, " \r\n", &tmpstr);
		// total_words += 1;
	}
	for(volatile uint64_t j{0}; j<extraworkXline; j++);
}

void reduce_maps(umap& map1, const umap& map2) {
	for(auto p : map2) {
		map1[p.first] += p.second;
	}
}

struct MapReduceDocs:ff_Map<vector_string, vector_string, vector_string> {
	using map=ff_Map<vector_string, vector_string, vector_string>;
	MapReduceDocs(vector_string filenames, long nw): ff_Map(nw), filenames(filenames) {}
	vector_string* svc(vector_string*) {
		
		vector_string* lines = new vector_string();
		
		map::parallel_reduce(*lines, vector_string{}, 0, filenames.size(), 
		[&](const long i, vector_string& lines) {
			// std::printf("Doc id: %d\n", i);
			std::string filename = filenames[i];
			std::string new_filename{filename};
			std::ifstream file(filename, std::ios_base::in);
			if (file.is_open()) {
				std::string line;
				while(std::getline(file, line)) {
					if (!line.empty()) {
						lines.push_back(line);
					}
				}
			}
			file.close();
		}, [](vector_string& lines, const vector_string& aux){
			lines.insert(lines.end(), aux.begin(), aux.end());
		});

		ff_send_out(lines);
		return EOS;
	}

	vector_string filenames;
};


struct MapReduceLines:ff_Map<vector_string, umap, umap> {
	using map=ff_Map<vector_string, umap, umap>;
	MapReduceLines(long nw): ff_Map(nw) {}
	umap* svc(vector_string* lines) {

		vector_string not_pointer_lines = *lines;
		map::parallel_reduce(UM, umap(), 0, not_pointer_lines.size(), 
		[&](const long i, umap& vUM) {
			tokenize_line(not_pointer_lines[i], vUM);
		}, [](umap& UM, const umap& aux){
			reduce_maps(UM, aux);
		});

		delete lines;
		return EOS;
	}

	umap UM;
};

int main(int argc, char *argv[]) {

	auto usage_and_exit = [argv]() {
		std::printf("use: %s filelist.txt [extraworkXline] [topk] [showresults]\n", argv[0]);
		std::printf("     filelist.txt contains one txt filename per line\n");
		std::printf("     extraworkXline is the extra work done for each line, it is an integer value whose default is 0\n");
		std::printf("     topk is an integer number, its default value is 10 (top 10 words)\n");
		std::printf("     showresults is 0 or 1, if 1 the output is shown on the standard output\n\n");
		exit(-1);
	};

	vector_string filenames;
	size_t topk = 10;
	bool showresults=false;
	if (argc < 2 || argc > 7) {
		usage_and_exit();
	}
	size_t num_threads_docs = std::stoul(argv[5]);
	size_t num_threads_lines = std::stoul(argv[6]);
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
			if (argc >= 5) {
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
	
	auto rng = std::default_random_engine{};
	std::shuffle(std::begin(filenames), std::end(filenames), rng);
	
	// used for storing results
	umap UM;

	ffTime(START_TIME);
	
	MapReduceDocs mpd(filenames, num_threads_docs);
	MapReduceLines mpl(num_threads_lines);

	ff_Pipe pipe(mpd, mpl);
    if (pipe.run_and_wait_end()<0) {
        error("running pipe");
        return -1;
    }
	ffTime(STOP_TIME);
	std::printf("Compute time (s) %f %f\n", ffTime(GET_TIME) / 1000, pipe.ffTime() / 1000);
	UM = mpl.UM;
	ffTime(START_TIME);
	ranking rank(UM.begin(), UM.end());
	ffTime(STOP_TIME);
	std::printf("Sorting time (s) %f\n", ffTime(GET_TIME) / 1000);
	
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