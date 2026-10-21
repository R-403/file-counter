file_counter: file_counter.cpp
	g++ -Wall -pthread file_counter.cpp -o file_counter

clean:
	rm -f file_counter
