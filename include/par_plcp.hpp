#pragma once
// par_plcp.hpp — parallel LCP array construction.
//
// Implements par-PLCP from:
//   Julian Shun, "Fast Parallel Computation of Longest Common Prefixes", SC14.
// which parallelizes the Karkkainen/Manzini/Puglisi PLCP reformulation
// (CPM09) of Kasai et al.'s linear-time LCP algorithm (CPM01).
//
// Usage, right after divsufsort():
//   divsufsort(text.data(), sa.data(), n);
//   std::vector<idx_t> lcp = par_plcp(text.data(), sa.data(), n);
// LCP[0] = 0, and for i>0, LCP[i] = length of the longest common prefix
// between the suffixes at sa[i-1] and sa[i] (i.e. standard SA-order LCP).

#include <cstddef>
#include <vector>
#include <omp.h>

template <typename sa_idx_t>
std::vector<sa_idx_t> par_plcp(const unsigned char* text, const sa_idx_t* sa, std::size_t n)
{
	if(n == 0) return {};

	// Phi[i] = the text position whose suffix immediately precedes suffix i
	// in SA order (or -1 for the lexicographically smallest suffix, which
	// has no predecessor). Computed as a parallel scatter over SA.
	// We reuse this same array to hold PLCP afterward -- once Phi[i] has
	// been read for position i, that slot is free to become PLCP[i].
	std::vector<sa_idx_t> plcp(n);
	std::vector<sa_idx_t>& phi = plcp;

	phi[sa[0]] = static_cast<sa_idx_t>(-1);
	#pragma omp parallel for schedule(static)
	for(std::size_t i = 1; i < n; ++i) {
		phi[sa[i]] = sa[i - 1];
	}

	// Split the text into K = (thread count) contiguous chunks and run the
	// sequential kmp-LCP scan independently in each. Restarting h = 0 at
	// every chunk boundary (not just at the one true Phi == -1 position)
	// is always safe -- LCP can only fall by at most 1 per text position,
	// so an overly conservative start just costs a few extra comparisons,
	// never a wrong answer. That's the whole trick, and the whole cost:
	// per Shun's analysis this adds O(K * lmax) work for the parallelism.
	const std::size_t K = static_cast<std::size_t>(omp_get_max_threads());
	#pragma omp parallel for schedule(static)
	for(std::size_t j = 0; j < K; ++j) {
		const std::size_t start = j * n / K;
		const std::size_t end = (j + 1) * n / K;
		sa_idx_t h = 0;
		for(std::size_t i = start; i < end; ++i) {
			if(phi[i] == static_cast<sa_idx_t>(-1)) {
				h = 0;
			} else {
				const std::size_t k = static_cast<std::size_t>(phi[i]);
				// Bounds-checked explicitly: unlike Shun's presentation,
				// we don't assume a sentinel character is appended to
				// text, so we stop at n ourselves instead of relying on
				// a mismatch guaranteed by a trailing $.
				while(i + h < n && k + h < n && text[i + h] == text[k + h]) {
					++h;
				}
			}
			plcp[i] = h;      // overwrites phi[i]; see comment above
			if(h > 0) --h;
		}
	}

	// PLCP -> LCP: PLCP is indexed by text position, LCP by SA rank.
	std::vector<sa_idx_t> lcp(n);
	lcp[0] = 0;
	#pragma omp parallel for schedule(static)
	for(std::size_t i = 1; i < n; ++i) {
		lcp[i] = plcp[sa[i]];
	}
	return lcp;
}
