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
#include <random>

using namespace ff;

using umap=std::unordered_map<std::string, uint64_t>;
using pair=std::pair<std::string, uint64_t>;
using first_task=std::string;
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

void reduce_maps(umap& map1, umap& map2) {
	for(auto p : map2) {
		map1[p.first] += p.second;
	}
}

struct separateDocs: ff_monode_t<std::string> {
    separateDocs(const std::vector<std::string> filenames):filenames(filenames) {}
    std::string* svc(std::string*) {
		
		for(auto filename: filenames) {
			ff_send_out(new std::string(filename));
		}
        return EOS;
    }
	
    std::vector<std::string> filenames;
};

struct extractLines: ff_monode_t<std::string, std::string> {
    std::string* svc(std::string* filename) {
		
		std::ifstream file(*filename, std::ios_base::in);
		if (file.is_open()) {
			std::string line;
			while(std::getline(file, line)) {
				if (!line.empty()) {
					ct += 1;
					ff_send_out(new std::string(line));
				}
			}
		}
		file.close();
        return GO_ON;
    }

	// void svc_end() {
	// 	std::printf("Docs: %d - %d\n", get_my_id(), ct);
	// }

	int ct{0};
};


struct tokenizeLines: ff_node_t<std::string, umap> {
    umap* svc(std::string* line) {
		umap vUM;
		std::string new_line = *line;
        tokenize_line(new_line, vUM);
		ct += 1;
		// std::printf("R-Worker %d: %d\n", get_my_id(), ct);
		delete line;
        return new umap{vUM};
    }

	// void svc_end() {
	// 	std::printf("Lines: %d - %d\n", get_my_id(), ct);
	// }

	int ct{0};
	umap vUM;
};


struct collectMaps: ff_minode_t<umap, umap> {
	umap* svc(umap* vUM) {
		// std::cout << "I am here! " << vUM->size() << '\n';
		reduce_maps(UM, *vUM);

		delete vUM;
        return this->GO_ON;
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

	std::vector<std::string> filenames;
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
	
	// auto rng = std::default_random_engine {};
	// std::shuffle(std::begin(filenames), std::end(filenames), rng);

	// used for storing results
	umap UM;

	ffTime(START_TIME);

	std::vector<ff_node*> L_workers;
	std::vector<ff_node*> R_workers;

	separateDocs first(filenames);
	collectMaps last;

	for(auto i=0;i<num_threads_docs;++i)
		L_workers.push_back(new extractLines());

	for(auto i=0;i<num_threads_lines;++i)
		R_workers.push_back(new tokenizeLines());

	ff_a2a a2a;

	a2a.add_firstset(L_workers);
	a2a.add_secondset(R_workers);
	
    ff_Pipe pipe(first, a2a, last);

    if (pipe.run_and_wait_end()<0) {
        error("running pipe");
        return -1;
    }
	ffTime(STOP_TIME);
	std::printf("Compute time (s) %f %f\n", ffTime(GET_TIME) / 1000, pipe.ffTime() / 1000);
	UM = last.UM;
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
	
