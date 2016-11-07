#pragma once
#include <thread>
#include <queue>
#include <condition_variable>

class ThreadPool {
public:
        // Constructor that takes in the number of threads and a file location
	// to retrieve the vendor addresses
	ThreadPool(int);
        // Destructor which simply joins the threads to clean up the memory
	// footprint of the pool
	~ThreadPool();
	// Method to submit work to the thread pool. The pool decides which
	// worker will use it
	int enqueue(std::function<void()>);
	enum Status { AVAILABLE, WORKING, WAITING };
private:
	void run(int i);
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;
        std::condition_variable condition;
	std::mutex queueMutex;
	std::vector<std::string> getAddresses(std::string addressesLocation);
};

