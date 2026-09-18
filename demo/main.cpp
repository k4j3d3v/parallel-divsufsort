#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>

#include <divsufsort.h>

#include <omp.h>
#include <CLI11.hpp>
#include <par_plcp.hpp>

using namespace std;

typedef int32_t num_type; // Currently only int32_t and int64_t are supported.

template <typename idx_t>
static void write_sa(const char* filename, const idx_t* p, std::size_t n)
{
	FILE* sa;

	if((sa = fopen(filename, "wb")) == NULL)  perror(filename);
	size_t c = fwrite(p, sizeof(*p), n, sa);
	if(c != n) {
		perror("Error writing the sa file");
		exit(1);
	}
	fclose(sa);
}

template <typename idx_t>
static bool run_par_divsufsort(const std::string& text, const std::string& sa_path, const std::string& lcp_path, bool generate_lcp)
{
	if(text.size() > static_cast<std::size_t>(std::numeric_limits<idx_t>::max())) {
		std::cerr << "Input is too large for the selected index width." << std::endl;
		return false;
	}

	std::vector<idx_t> sa(text.size());

	auto start = chrono::steady_clock::now();
	divsufsort((sauchar_t*)text.data(), sa.data(), text.size());

	auto end = chrono::steady_clock::now();
	auto diff = end - start;
	cout << "Parallel DSS time: " <<
		chrono::duration<double, milli>(diff).count() << " ms" << endl;

	
	//if (sufcheck((sauchar_t*)text.data(), sa.data(), text.size(), false)) {
	//	cout << "Sufcheck failed!" << endl;
	//	return false;
	//}
	if(!sa_path.empty()) {
		write_sa(sa_path.c_str(), sa.data(), static_cast<std::size_t>(text.size()));
	}

	if(!lcp_path.empty() || generate_lcp) {
		auto lcp_start = chrono::steady_clock::now();
		par_plcp<idx_t>((sauchar_t*)text.data(), sa, text.size());
		auto lcp = sa; // par_plcp returns the LCP array in the same vector as SA, so we can just copy it here.
		auto lcp_end = chrono::steady_clock::now();
		cout << "par_plcp time: " <<
			chrono::duration<double, milli>(lcp_end - lcp_start).count() << " ms" << endl;
		
		if (generate_lcp) {
			double avg_lcp = 0.0;
			for(std::size_t i = 1; i < lcp.size(); ++i) {
				avg_lcp += static_cast<double>(lcp[i]);
			}

			avg_lcp /= static_cast<double>(lcp.size() - 1);
			cout << "Average LCP: " << avg_lcp << endl;
		}
		if (!lcp_path.empty()) {
			write_sa(lcp_path.c_str(), lcp.data(), lcp.size());   // same binary layout as the SA file
		}
	}
	return true;
}

int main(int argc, char* args[]) {
	
	CLI::App app{"parallel-divsufsort driver"};
	std::string input_path;
	app.add_option("input", input_path, "input path")->required();

	std::string sa_path;
	app.add_option("-w,--output", sa_path, "output SA file path")->default_val("");

	std::string lcp_path;
	app.add_option("-W,--lcp-path", lcp_path, "output LCP array file path")->default_val("");
	
	bool generate_lcp = false;
	app.add_flag("-a,--avglcp", generate_lcp, "compute LCP array and print the average LCP")->default_val("false");

	std::size_t threads = 1;
	app.add_option("-t,--threads", threads, "number of threads to use")->default_val("1");

	CLI11_PARSE(app, argc, args);

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
		ifstream input_file(input_path);
		input_file.seekg(0, ios::end);   
		text.reserve(input_file.tellg());
		input_file.seekg(0, ios::beg);
		text.assign((istreambuf_iterator<char>(input_file)),
				istreambuf_iterator<char>());
	}

	if(text.size() <= static_cast<std::size_t>(std::numeric_limits<uint32_t>::max())) {
		return run_par_divsufsort<int32_t>(text, sa_path, lcp_path, generate_lcp) ? 0 : -1;
	}
	return run_par_divsufsort<int64_t>(text, sa_path, lcp_path, generate_lcp) ? 0 : -1;

	// num_type *SA = new num_type[size];
	// for (int i = 0; i < times; ++i) {
	// 	auto start = chrono::steady_clock::now();
	// 	divsufsort((sauchar_t*)text.data(), SA, size);
	// 	auto end = chrono::steady_clock::now();
	// 	auto diff = end - start;
	// 	cout <<	chrono::duration <double, milli> (diff).count() / 1000.0 << ", ";
	// 	cout.flush();
	// }

	// return 0;
}
