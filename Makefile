all: clean
	@g++ mt_test.cpp -Wall -Ofast -march=native -lpthread -o mt_test -std=gnu++11

run: all
	@./mt_test

clean:
	@rm mt_test -f
