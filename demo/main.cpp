#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>

#include <divsufsort.h>
#include <omp.h>

using namespace std;

typedef int32_t num_type; // Currently only int32_t and int64_t are supported.
int times = 3; // How often the time measurement is repeated.

size_t getPeakRSS() {
	struct rusage rusage;
	getrusage( RUSAGE_SELF, &rusage);
	return (size_t)(rusage.ru_maxrss * 1024L);
}

int main(int argc, char* args[]) {
	
	CLI::App app{"parallel-range-lite driver"};
	std::string input_path;
	app.add_option("input", input_path, "input path")->required();

	std::string sa_path;
	app.add_option("-w,--output", sa_path, "output SA file path")->default_val("");

	std::string symbol_width = "8";
	app.add_option("-b,--bits", symbol_width, "Symbol width (8, 16, or 32)")
		->check(CLI::IsMember({"8", "16", "32"}))
		->default_val("8");

	std::size_t threads = 1;
	app.add_option("-t,--threads", threads, "number of threads to use")->default_val("1");

	CLI11_PARSE(app, argc, argv);

	if(threads == 0) {
		std::cerr << "Thread count must be positive." << std::endl;
		return -1;
	}
	if(threads > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
		std::cerr << "Thread count is too large." << std::endl;
		return -1;
	}

	omp_set_num_threads(static_cast<int>(threads));

	string text;
	{ // Read input file.
		ifstream input_file(args[1]);
		input_file.seekg(0, ios::end);   
		text.reserve(input_file.tellg());
		input_file.seekg(0, ios::beg);
		text.assign((istreambuf_iterator<char>(input_file)),
				istreambuf_iterator<char>());
	}
	num_type size = text.size();
	num_type *SA = new num_type[size];
	for (int i = 0; i < times; ++i) {
		auto start = chrono::steady_clock::now();
		divsufsort((sauchar_t*)text.data(), SA, size);
		auto end = chrono::steady_clock::now();
		auto diff = end - start;
		cout <<	chrono::duration <double, milli> (diff).count() / 1000.0 << ", ";
		cout.flush();
	}
	cout << getPeakRSS() / (1024*1024)<< endl;
	if (sufcheck((sauchar_t*)text.data(), SA, size, false)) {
		cout << "Sufcheck failed!" << endl;
		return -1;
	}
	return 0;
}
