//
// Sequential code of the first SPM Assignment a.a. 23/24.
//
// compile:
// g++ -std=c++20 -O3 -march=native -I<path-to-include> UTWavefront.cpp -o UTW
//
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>
#include <hpc_helpers.hpp>
#include <barrier>

std::vector<std::thread> threads;
int num_threads = 8;
int block = 1;

int random(const int &min, const int &max) {
	static std::mt19937 generator(117);
	std::uniform_int_distribution<int> distribution(min,max);
	return distribution(generator);
};		

// emulate some work
void work(std::chrono::microseconds w) {
	auto end = std::chrono::steady_clock::now() + w;
    while(std::chrono::steady_clock::now() < end);	
}

void wavefront(const std::vector<int> &M, const uint64_t &N) {
	for(uint64_t k = 0; k< N; ++k) {        // for each upper diagonal
		for(uint64_t i = 0; i< (N-k); ++i) {// for each elem. in the diagonal
			work(std::chrono::microseconds(M[i*N+(i+k)])); 
		}
	}
}

int main(int argc, char *argv[]) {
	int min    = 0;      // default minimum time (in microseconds)
	int max    = 1000;   // default maximum time (in microseconds)
	uint64_t N = 512;    // default size of the matrix (NxN)

	if (argc != 1 && argc != 2 && argc != 4 && argc != 5 && argc != 6) {
		std::printf("use: %s N [min max num_threads]\n", argv[0]);
		std::printf("     N size of the square matrix\n");
		std::printf("     min waiting time (us)\n");
		std::printf("     max waiting time (us)\n");
		std::printf("     number of threads\n");
		std::printf("     cycle(0) or block(1)\n");
		return -1;
	}
	if (argc > 1) {
		N = std::stol(argv[1]);
		if (argc > 2) {
			min = std::stol(argv[2]);
			max = std::stol(argv[3]);
			if(argc > 4) {
				num_threads = std::stol(argv[4]);
				if(argc > 5)
					block = std::stol(argv[5]);
			}
		}
	}
	std::barrier threads_barrier{num_threads};
	
	// allocate the matrix
	std::vector<int> M(N*N, -1);

	uint64_t expected_totaltime=0;
	// init function
	auto init=[&]() {
		for(uint64_t k = 0; k< N; ++k) {
			for(uint64_t i = 0; i< (N-k); ++i) {  
				int t = random(min,max);
				M[i*N+(i+k)] = t;
				expected_totaltime +=t;				
			}
		}
	};
	
	init();

	auto wavefront_block = [&](uint64_t id) {
		int actual_num_threads = num_threads;
		for(uint64_t k = 0; k < N - id; ++k) {        // for each upper diagonal
			if(num_threads > N - k)
				actual_num_threads = N - k;
			
			uint64_t modulo = (N - k) % actual_num_threads;
			uint64_t add_start = std::min(modulo, id);
			uint64_t add_end = std::min(modulo, id + 1);
			uint64_t start = (N - k) / actual_num_threads * id + add_start;
			uint64_t end = (N - k) / actual_num_threads * (id + 1) + add_end;
			// std::printf("Diagonal size: %d; Start: %d; End: %d; Id: %d\n", N - k, start, end, id);
			for(uint64_t i = start; i< end; ++i) {	// for each elem. in the diagonal	
				work(std::chrono::microseconds(M[i*N+(i+k)])); 
			}
			threads_barrier.arrive_and_wait();
		}
		for(uint64_t k = N - id; k < N; k++) {
			threads_barrier.arrive_and_wait();
		}
	};

	auto wavefront_cycle = [&](uint64_t id) {
		for(uint64_t k = 0; k < N - id; ++k) {        // for each upper diagonal
			for(uint64_t i = 0; i<(N-k); i+=num_threads) {	// for each elem. in the diagonal	
				work(std::chrono::microseconds(M[i*N+(i+k)])); 
			}
			threads_barrier.arrive_and_wait();
		}
		for(uint64_t k = N - id; k < N; k++) {
			threads_barrier.arrive_and_wait();
		}
	};

	if(num_threads != 1)
		std::printf("Estimated compute time ~ %f (s)\n", expected_totaltime/1000000.0);
	std::printf("%d ", num_threads);

	if(block) {
		TIMERSTART(wavefront_block);

		for(uint64_t k = 0; k < num_threads; ++k)
		{
			uint64_t id = k;
			threads.emplace_back(wavefront_block, id);
		}
		for(uint64_t k = 0; k < num_threads; ++k)
		{
			threads[k].join();
		}
		// wavefront(M, N); 
		TIMERSTOP(wavefront_block);
	}
	else {
		TIMERSTART(wavefront_cycle);

		for(uint64_t k = 0; k < num_threads; ++k)
		{
			uint64_t id = k;
			threads.emplace_back(wavefront_cycle, id);
		}
		for(uint64_t k = 0; k < num_threads; ++k)
		{
			threads[k].join();
		}
		// wavefront(M, N); 
		TIMERSTOP(wavefront_cycle);
	}
    return 0;
}
