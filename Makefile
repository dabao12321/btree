TARGET := test_btree 
CXX = clang++ -std=c++17 -L/home/ubuntu/xvdf_mounted/cilkrts/build/install/lib
CXXFLAGS = -O3 -march=native -Wall -g -m64 -I. 

ifeq ($(CILK),1)
  CXXFLAGS += -fcilkplus -DCILK=1
endif

test_btree: test_btree.cpp
	$(CXX) test_btree.cpp -o test_btree.o $(CXXFLAGS)

clean :
	rm *.o
