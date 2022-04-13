#include "btree.h"

#include <random>
#include <vector>
#include <algorithm>
#include <functional>
#include <set>
#include <sys/time.h>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
// #include <parallel.h>

#define PARALLEL 0

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

  // printf("\ncorrect sum: ,\t %lu,", std::accumulate(inserted_data.begin(), inserted_data.end(), 0L));

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

#if PARALLEL
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
  test_btree_unordered_insert<uint64_t>(100000000, seed, times);
	printf("\ninsert time %lu, find time %lu, sumiter time %lu, sum time %lu\n", times[0], times[1], times[2], times[3]);
#endif
	return 0;
}

