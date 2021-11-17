#include "btree.h"

#include <random>
#include <vector>
#include <algorithm>
#include <functional>
#include <set>
#include <sys/time.h>

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
void test_btree_unordered_insert(uint64_t max_size, std::seed_seq &seed) {
  if (max_size > std::numeric_limits<T>::max()) {
    max_size = std::numeric_limits<T>::max();
  }
  std::vector<T> data =
      create_random_data<T>(max_size, std::numeric_limits<T>::max(), seed);
  // std::set<T> inserted_data;

  uint64_t start, end;
  BTree<T, T> s;
  start = get_usecs();
  for (uint32_t i = 1; i < max_size; i++) {
    s.insert(data[i]);
    // inserted_data.insert(data[i]);
  }
  end = get_usecs();


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

int main() {
  // test_btree_ordered_insert<uint32_t>(129);
  std::seed_seq seed{0};
  printf("------- ORDERED INSERT --------");
  test_btree_ordered_insert<uint64_t>(100000000);
  printf("------- UNORDERED INSERT --------");
  test_btree_unordered_insert<uint64_t>(100000000, seed);
}