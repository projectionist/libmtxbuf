.PHONY: clean

tests/buffer:
	$(CXX) -std=c++14 -pthread -Wall -Wextra -O3 -Isrc -o $@_test $@.cpp

clean:
	rm -rf tests/buffer_test
