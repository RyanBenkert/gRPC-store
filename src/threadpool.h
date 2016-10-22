#pragma once
#include <thread>
#include <queue>

class ThreadPool {
public:
        // Constructor that takes in the number of threads and a vector of
	// addresses (for vendors)
	ThreadPool(size_t, std::vector<std::string>);
        // Destructor which simply joins the threads to clean up the memory
	// footprint of the pool
	~ThreadPool();
	// Method to submit work to the thread pool. The pool decides which
	// worker will use it
	int enqueue(std::function<void()>);
private:
	// need to keep track of threads so we can join them
	std::vector<std::thread> workers;
	//     // the task queue
	std::queue<std::function<void()>> tasks;
};

