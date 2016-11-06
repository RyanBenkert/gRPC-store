#include "threadpool.h"
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include <grpc++/grpc++.h>
#include "store.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using store::Store;

class StoreImpl {
public:
	~StoreImpl() {
		server_->Shutdown();
		cq_->Shutdown();
	}

	void RunServer(std::string portNumber, ThreadPool *pool) {
		ServerBuilder builder;
		std::string server_address("0.0.0.0:" + portNumber);
		builder.AddListeningPort(server_address,
					 grpc::InsecureServerCredentials());
		builder.RegisterService(&service_);
		cq_ = builder.AddCompletionQueue();
		server_ = builder.BuildAndStart();
		std::cout << "Starting to run under " << server_address << "\n";
		while(true) {
			pool->enqueue( []() { std::cout << "おはよう" << std::endl; });
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

private:
	Store::AsyncService service_;
	std::unique_ptr<ServerCompletionQueue> cq_;
	std::unique_ptr<Server> server_;
};


int main(int argc, char** argv) {
	int nrOfThreads;
        std::string addressesList, portNumber;
	if (argc == 4) {
		addressesList = std::string(argv[1]);
                nrOfThreads = atoi(argv[2]);
		portNumber = argv[3];
	} else {
		std::cerr << "Usage: ./store vendorAddresses nrOfThreads portNumber\n" << std::endl;
		return EXIT_FAILURE;
	}
        // Create pool before running server to ensure there are always threads
	// available at the beginning of execution
	ThreadPool *pool = new ThreadPool(addressesList, nrOfThreads);
	StoreImpl store;
	store.RunServer(portNumber, pool);
}
