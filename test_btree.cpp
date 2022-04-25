#include "btree.h"

#include <random>
#include <vector>
#include <algorithm>
#include <functional>
#include <set>
#include <sys/time.h>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <parallel.h>

#define PARALLEL_RUNS 0
#define PARALLEL_FIND_SUM 1

static long get_usecs() {
    struct timeval st;
    gettimeofday(&st,NULL);
    return st.tv_sec*1000000 + st.tv_usec;
}

template <class T>
std::vector<T> create_random_data(size_t n, size_t max_val,
                                  std::seed_seq &seed) {

  std::mt19937_64 eng(seed); // a source of random data

  std::uniform_int_distribution<T> dist(0, max_val);
  std::vector<T> v(n);

  generate(begin(v), end(v), bind(dist, eng));
  return v;
}

template <class T> void test_btree_ordered_insert(uint64_t max_size) {
  if (max_size > std::numeric_limits<T>::max()) {
    max_size = std::numeric_limits<T>::max();
  }
  uint64_t start, end;
  BTree<T, T> s;

  start = get_usecs();
  for (uint32_t i = 1; i < max_size; i++) {
    s.insert(i);
  }
  end = get_usecs();
  printf("\ninsertion,\t %lu,", end - start);

  start = get_usecs();
  for (uint32_t i = 1; i < max_size; i++) {
    auto node = s.find(i);
    if (node == nullptr) {
      printf("\ncouldn't find data in btree at index %u\n", i);
      exit(0);
    }
  }
  end = get_usecs();
  printf("\nfind all,\t %lu,", end - start);

  start = get_usecs();
  uint64_t sum = 0;
  auto it = s.begin();
  while (!it.done()) {
    T el = *it;
    sum += el;
    ++it;
  }
  end = get_usecs();
  printf("\nsum_time with iterator, \t%lu, \tsum_total, \t%lu, \t", end - start,
         sum);
  start = get_usecs();
  sum = s.sum();
  end = get_usecs();
  printf("\nsum_time, %lu, sum_total, %lu\n", end - start, sum);
}

template <class T>
void test_btree_unordered_insert(uint64_t max_size, std::seed_seq &seed, uint64_t* times) {
  if (max_size > std::numeric_limits<T>::max()) {
    max_size = std::numeric_limits<T>::max();
  }
  std::vector<T> data =
      create_random_data<T>(max_size, std::numeric_limits<T>::max(), seed);
      //create_random_data<T>(max_size, 100, seed);

  // std::set<T> inserted_data;

  uint64_t start, end;

  // save insertion, find, iter sum, naive sum times

  BTree<T, T> s;
  start = get_usecs();
  for (uint32_t i = 1; i < max_size; i++) {
    s.insert(data[i]);
    // inserted_data.insert(data[i]);
  }
  end = get_usecs();

  times[0] = end - start;
  printf("\ninsertion,\t %lu,", end - start);

#if STATS
  int levels = s.get_num_levels();
  int num_nodes = s.get_num_nodes();
  int num_internal_nodes = s.get_num_internal_nodes();
  int num_internal_elems = s.get_num_internal_elements();
  float internal_density = (num_internal_elems / num_internal_nodes)/float(MAX_KEYS);
  float leaf_density = ((max_size - num_internal_elems) / (num_nodes - num_internal_nodes)) / float(MAX_KEYS);

  printf("\ndensity stats: \n\tnum_levels: %u\n\tnum nodes: %u\n\tinternal node density: %f\n\tleaf node density: %f",
          levels, num_nodes, internal_density, leaf_density);

  printf("\n\tspace density: %f", (float)max_size * sizeof(uint64_t) / s.get_size());

  printf("\navg number of linear comparisons made per insert: %lu", s.get_total_comparisons()/max_size);

  printf("\n");
  return;
#endif

  // printf("\ncorrect sum: ,\t %lu,", std::accumulate(inserted_data.begin(), inserted_data.end(), 0L));

#if PARALLEL_FIND_SUM
  // PARALLEL FIND AND SUM
  	// parallel find
  std::seed_seq seed2{1};

	// generate n / 10 random elts
  std::vector<T> data_to_search =
      create_random_data<T>(max_size / 10, std::numeric_limits<T>::max(), seed2);

	// pick n/10 from the input
	for(uint32_t i = 0; i < max_size; i+=10) {
		if (i < max_size) { data_to_search.push_back(data[i]); }
	}

	// shuffle them
  std::mt19937_64 g(seed); // a source of random data
	std::shuffle(data_to_search.begin(), data_to_search.end(), g);

	std::vector<T> partial_sums(getWorkers() * 8);

  start = get_usecs();
  parallel_for (uint32_t i = 0; i < data_to_search.size(); i++) {
    auto node = s.find(data_to_search[i]);
		
		partial_sums[getWorkerNum() * 8] += !(node == nullptr);
  }

  end = get_usecs();

	uint64_t parallel_find_time = end - start;

	// sum up results
	T result = 0;
	for(int i = 0; i < getWorkers(); i++) {
		result += partial_sums[i * 8];
	}

  printf("\nparallel find,\t %lu,\tnum found %lu\n", parallel_find_time, result);

  start = get_usecs();
  uint64_t psum = s.psum();
  end = get_usecs();
  uint64_t sum = s.sum();
  printf("\nparallel sum, %lu, psum_total, %lu \tsum_total, %lu\n", end - start, psum, sum);

#else
  // SERIAL FIND AND SUM
  start = get_usecs();
  for (uint32_t i = 1; i < max_size; i++) {
    auto node = s.find(data[i]);
    if (node == nullptr) {
      printf("\ncouldn't find data in btree at index %u\n", i);
      exit(0);
    }
  }
  end = get_usecs();

  times[1] = end - start;
  printf("\nfind all,\t %lu,", end - start);

  start = get_usecs();
  uint64_t sum = 0;
  auto it = s.begin();
  while (!it.done()) {
    T el = *it;
    sum += el;
    ++it;
  }
  
  end = get_usecs();
  times[2] = end - start;
  printf("\nsum_time with iterator, \t%lu, \tsum_total, \t%lu, \t", end - start,
         sum);

  start = get_usecs();
  sum = s.sum();
  end = get_usecs();
  times[3] = end - start;
  printf("\nsum_time, %lu, sum_total, %lu\n", end - start, sum);
#endif

}

int main() {
  printf("B tree node internal size %zu\n", sizeof(BTreeNodeInternal<uint64_t,uint64_t>));
  printf("B tree node leaf size %zu\n", sizeof(BTreeNodeLeaf<uint64_t,uint64_t>));
  // test_btree_ordered_insert<uint32_t>(129);
  
  std::seed_seq seed{0};
  // printf("------- ORDERED INSERT --------\n");
  // test_btree_ordered_insert<uint64_t>(100000000);
  printf("------- UNORDERED INSERT --------\n");
  uint64_t times[4];

#if PARALLEL_RUNS
  // MULTIPLE PARALLEL RUNS, STATS ARE AVERAGED

  // IF YOU SET THIS TO BE ANYTHING 2 - 6, IT GIVES THE CLANG WEIRD BUG
  uint64_t num_parallel = 7;
  
  uint64_t insert_times[num_parallel];
  uint64_t find_times[num_parallel];
  uint64_t sumiter_times[num_parallel];
  uint64_t sum_times[num_parallel];

  uint64_t insert_total = 0;
  uint64_t find_total = 0;
  uint64_t sumiter_total = 0;
  uint64_t sum_total = 0;

  // set to 16 workers
  // __cilkrts_set_param("nworkers","16");

  cilk_for (int i = 0; i < num_parallel; i++) {
    uint64_t times[4];
    test_btree_unordered_insert<uint64_t>(100000000, seed, times);
    // test_btree_ordered_insert<uint64_t>(100000);
    insert_times[i] = times[0];
    find_times[i] = times[1];
    sumiter_times[i] = times[2];
    sum_times[i] = times[3];
  }

  for (int i = 0; i < num_parallel; i++) {
    insert_total += insert_times[i];
    find_total += find_times[i];
    sumiter_total += sumiter_times[i];
    sum_total += sum_times[i];
  }

  printf("\n------- AVERAGE TIMES --------\n");


  printf("\ninsertion %lu \nfind all %lu \nsum iter %lu \nsum native %lu \n",
        (insert_total/num_parallel),
        (find_total/num_parallel),
        (sumiter_total/num_parallel),
        (sum_total/num_parallel));
#else 
  // SINGLE RUN
  test_btree_unordered_insert<uint64_t>(1000000, seed, times);
  // test_btree_unordered_insert<uint64_t>(100000000, seed, times);
	// printf("\ninsert time %lu, find time %lu, sumiter time %lu, sum time %lu\n", times[0], times[1], times[2], times[3]);
#endif
	return 0;
}

