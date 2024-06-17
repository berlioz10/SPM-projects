#include <cstdio>
#include <random>
#include <map>
#include <vector>
#include <string>
#include <mpi.h>
#include <unistd.h>
#include <fstream>

const long SIZE = 64;

long random(const int &min, const int &max) {
	static std::mt19937 generator(117);
	std::uniform_int_distribution<long> distribution(min,max);
	return distribution(generator);
};		

void init(auto& M, const long c1, const long c2, const long key) {
	for(long i=0;i<c1;++i)
		for(long j=0;j<c2;++j)
			M[c2 * i + j] = (key-i-j)/static_cast<float>(SIZE);
}

// matrix multiplication:  C = A x B  A[c1][c2] B[c2][c1] C[c1][c1]
// mm returns the sum of the elements of the C matrix
auto mm(const auto& A, const auto& B, const long c1,const long c2, int end) {

	float sum{0};
    float accum{0};
	
    for (long i = 0; i < end; i++) {
        for (long j = 0; j < c1; j++) {
            accum = 0.0;
            for (long k = 0; k < c2; k++) {
                accum += A[i * c2 + k] * B[k * c1 + j];
            }
            sum += accum;
		}
	}
	return sum;
}

float compute(const long c1, const long c2, long key1, long key2, int myId, int numP) {

	std::vector<float> A;
	std::vector<float> B;
    int end = 0;
	float r;
    if(myId == 0) {
		
		int modulo = c1 % numP;
		end = c1 / numP * (myId + 1) + std::min(modulo, myId + 1);

		A = std::vector<float>(c1 * c2, 0.0);
		B = std::vector<float>(c2 * c1, 0.0);
		std::string stream;
		
        init(A, c1, c2, key1);
        init(B, c2, c1, key2);

        for(int i = 1; i < numP; i ++) {
            int start_Id = c1 / numP * i + std::min(modulo, i);
            int end_Id = c1 / numP * (i + 1) + std::min(modulo, i + 1);
            int how_many = end_Id - start_Id;
			std::vector<long> send_to = {c1, c2, how_many};
			MPI_Send(send_to.data(), 3, MPI_LONG, i, 0, MPI_COMM_WORLD);

            MPI_Send(&A[start_Id * c2], how_many * c2, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
            MPI_Send(B.data(), c2 * c1, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
        }
		r = mm(A, B, c1, c2, end);
    }
    else {
		long b1, b2, how_many;
		std::vector<long> recv = {0, 0, 0};
		MPI_Recv(recv.data(), 3, MPI_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if(recv[0] == -1) {
			return -1.0;
		}
		b1 = recv[0];
		b2 = recv[1];
		how_many = recv[2];
		A = std::vector<float>(how_many * b2, 0.0);
		B = std::vector<float>(b2 * b1, 0.0);
        MPI_Recv(A.data(), how_many * b2, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(B.data(), b2 * b1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		r = mm(A, B, b1, b2, how_many);
    }
    float sum = 0.0;
    MPI_Reduce(&r, &sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
	
	return sum;
}



int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::printf("use: %s nkeys length [print(0|1)]\n", argv[0]);
		std::printf("     print: 0 disabled, 1 enabled\n");
		return -1;
	}

	MPI_Init(&argc,&argv);
	
	double t1, t2;

	t1 = MPI_Wtime();
	
	// Get the number of processes
	int numP, myId, namelen; 
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Comm_size(MPI_COMM_WORLD,&numP);
	MPI_Get_processor_name(processor_name,&namelen);
	MPI_Comm_rank(MPI_COMM_WORLD, &myId);


	long nkeys  = std::stol(argv[1]);  // total number of keys
	// length is the "stream length", i.e., the number of random key pairs
	// generated
	long length = std::stol(argv[2]);  
	bool print = false;
	if (argc == 4)
		print = (std::stoi(argv[3]) == 1) ? true : false;
	
	long key1, key2;

	std::map<long, long> map;
	for(long i=0;i<nkeys; ++i) map[i]=0;
	
	std::vector<float> V(nkeys, 0);
	bool resetkey1=false;
	bool resetkey2=false;
	if(myId == 0) {

		if(true) {
			for(int i=0; i<length; ++i) {
				key1 = random(0, nkeys-1);  // value in [0,nkeys)
				key2 = random(0, nkeys-1);  // value in [0,nkeys)
				
				// we only need the results on master process
				if (key1 == key2) // only distinct values in the pair
					key1 = (key1+1) % nkeys; 
				map[key1]++;  // count the number of key1 keys
				map[key2]++;  // count the number of key2 keys

				float r1;
				float r2;
				// if key1 reaches the SIZE limit, then do the computation and then
				// reset the counter ....
				if (map[key1] == SIZE && map[key2]!=0) {
					// tell the other communicator we have to calculate this:
					r1 = compute(map[key1], map[key2], key1, key2, myId, numP);
					V[key1] += r1;
					resetkey1=true;	
				}
				// if key2 reaches the SIZE limit ....
				if (map[key2] == SIZE && map[key1]!=0) {			
					r2= compute(map[key2], map[key1], key2, key1, myId, numP);
					V[key2] += r2;  // sum the partial values for key1
					resetkey2=true;
				}
				if (resetkey1) {
					// updating the map[key1] initial value before restarting
					// the computation
					auto _r1 = static_cast<unsigned long>(r1) % SIZE;
					map[key1] = (_r1>(SIZE/2)) ? 0 : _r1;
					resetkey1 = false;
				}
				if (resetkey2) {
					// updating the map[key2] initial value before restarting
					// the computation
					auto _r2 = static_cast<unsigned long>(r2) % SIZE;
					map[key2] = (_r2>(SIZE/2)) ? 0 : _r2;
					resetkey2 = false;
				}
			}

			// compute the last values
			for(long i=0;i<nkeys; ++i) {
				for(long j=0;j<nkeys; ++j) {
					if (i==j) continue;
					if (map[i]>0 && map[j]>0) {
						auto r1 = compute(map[i], map[j], i, j, myId, numP);
						auto r2 = compute(map[j], map[i], j, i, myId, numP);
						V[i] += r1;
						V[j] += r2;
					}
				}
			}
		}
		// exit
	
		std::vector<long> exit_vector = {-1, -1, -1};

		for(int i = 1; i < numP; i ++) {
			MPI_Send(exit_vector.data(), 3, MPI_LONG, i, 0, MPI_COMM_WORLD);
		}
	} else {
		bool ok = true;
		while(ok) {
			float r = compute(0, 0, 0, 0, myId, numP);
			if(r == -1.0)
				ok = false;
		}
	}
	
	MPI_Finalize();

	if (print && !myId) {
		for(long i=0;i<nkeys; ++i)
			std::printf("key %ld : %f\n", i, V[i]);
	}

	t2 = MPI_Wtime();
	if(myId == 0)
		std::printf("Time: %f\n", t2 - t1);

	return 0;
}
