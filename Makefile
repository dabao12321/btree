TARGET := test_btree 
CXX = clang++ -std=c++17
CXXFLAGS = -Wall -g -m64 -I. 

ifeq ($(CILK),1)
  CXXFLAGS += -fcilkplus
endif

test_btree: test_btree.cpp
	$(CXX) test_btree.cpp -o test_btree.o $(CXXFLAGS)

clean :
	rm *.o