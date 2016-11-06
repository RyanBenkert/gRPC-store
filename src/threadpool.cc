#include "threadpool.h"
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include "vendor.grpc.pb.h"
using grpc::Channel;

std::vector<std::thread> workers;

ThreadPool::ThreadPool(std::string addressesLocation, int nrOfThreads) {
	for(std::string address : getAddresses(addressesLocation)) {
		std::cout << "Vendor addresses: " << address << std::endl;
	}
	for(int i = 0;i < nrOfThreads; i++) {
		workers.emplace_back([this, i] { run(i); });
		std::cout << "Thread number " + std::to_string(i) + " is ready" << std::endl;
	}
}

void ThreadPool::run(int i) {
	thread_local Status status;
	while(true) {
		status = Status::AVAILABLE;
		std::function<void()> task;
		std::unique_lock<std::mutex> lock(queueMutex);

                status = Status::WAITING;
		condition.wait(lock, [this]{ return !tasks.empty();});
                status = Status::WORKING;
		task = tasks.front();
                tasks.pop();
		lock.unlock();

		// Execute it
		task();
                std::cout << "- from thread " << i << std::endl;
	}
}

int ThreadPool::enqueue(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(queueMutex);
	tasks.push(task);
	std::cout << "Task enqueued: " << tasks.size() << " waiting to run" << std::endl;
	lock.unlock();
	// Notify one thread (any) about it
	condition.notify_one();
	// Return 0 if successful 1 otherwise
}

std::vector<std::string> ThreadPool::getAddresses(std::string addressesLocation) {
	std::vector<std::string> ipAddresses;
	std::ifstream vendorsFile (addressesLocation);
	int addrIndex = -1;
	if (vendorsFile.is_open()) {
		std::string ipAddr;
		while (getline(vendorsFile, ipAddr)) {
			if (addrIndex == -1) {
				ipAddresses.push_back(ipAddr);
			} else if (addrIndex == 0) {
				ipAddresses.push_back(ipAddr);
				break;
			} else {
				--addrIndex;
			}
		}
		vendorsFile.close();
		return ipAddresses;
	} else {
		std::cerr << "Failed to open file " << addressesLocation << std::endl;
		return {};
	}
}
