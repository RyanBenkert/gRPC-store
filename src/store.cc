#include "threadpool.h"
#include <string>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include <grpc++/grpc++.h>
#include "store.grpc.pb.h"
#include "vendor.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using store::Store;
using store::ProductInfo;
using store::ProductReply;
using store::ProductQuery;

class StoreImpl final {
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
		this->pool_ = pool;
		std::cout << "Starting to run under " << server_address << std::endl;
		HandleRpcs();
                // Just a way to test the pool...
		//while(true) {
			//pool->enqueue( []() { std::cout << "おはよう" << std::endl; });
			//std::this_thread::sleep_for(std::chrono::milliseconds(500));
		//}
	}
private:
        // Implementation partly borrowed from gRPC greeter async service
	// helloworld example
	class CallData {
	public:
		CallData(Store::AsyncService* service, ServerCompletionQueue* cq,
			 ThreadPool *pool)
			: service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
				this->pool_ = pool;
				Proceed();
			}

		void Proceed() {
			if (status_ == CREATE) {
				std::cout << "CREATE: Roger that." << std::endl;
				// Start processing the request getProducts,
				// 'this' identifies CallData instances
				service_->RequestgetProducts(&ctx_,
							     &request_,
							     &responder_,
							     cq_,
							     cq_,
							     this);
				std::cout << " - Started processing Request GetProduct." << std::endl;
				status_ = PROCESS;
			} else if (status_ == PROCESS) {
				// Spawn a new CallData instance to serve new clients while we process
				// the one for this CallData. The instance will deallocate itself as
				// part of its FINISH state.
				std::cout << "Creating alternative CallData" << std::endl;
				new CallData(service_, cq_, pool_);

				// Enqueue in pool to start sending requests to
				// clients
				std::string query = request_.product_name();
				std::future<std::vector<VendorBid>> futureQuery = pool_->appendQuery(query);
				std::vector<VendorBid> vendorBids = futureQuery.get();
				for(VendorBid bid : vendorBids) {
					ProductInfo* product_info = reply_.add_products();
					std::cout << "Bid: (" << query << ", " << bid.vendor_id<< ", " << bid.price << ")" << std::endl;
					product_info->set_price(bid.price);
					product_info->set_vendor_id(bid.vendor_id);
				}

				std::cout << "Received response for: " << request_.product_name() << std::endl;
				status_ = FINISH;
				responder_.Finish(reply_, Status::OK, this);
			} else {
				GPR_ASSERT(status_ == FINISH);
				delete this;
			}
		}

	private:
		// The means of communication with the gRPC runtime for an asynchronous
		// server.
		Store::AsyncService* service_;
		// The producer-consumer queue where for asynchronous server notifications.
		ServerCompletionQueue* cq_;
		// Context for the rpc, allowing to tweak aspects of it such as the use
		// of compression, authentication, as well as to send metadata back to the
		// client.
		ServerContext ctx_;
                // What we get from the client
		ProductQuery request_;
		// What we send back to the client
		ProductReply reply_;
		// The means to get back to the client.
		ServerAsyncResponseWriter<ProductReply> responder_;
		// Let's implement a tiny state machine with the following states.
		enum CallStatus { CREATE, PROCESS, FINISH };
		CallStatus status_;  // The current serving state.
	        ThreadPool *pool_;
	};

	void HandleRpcs() {
		new CallData(&service_, cq_.get(), pool_);
		void* tag;  // uniquely identifies a request.
		bool ok;
		while(true) {
			std::cout << "Waiting for client request" << "\n";
			GPR_ASSERT(cq_->Next(&tag, &ok));
			std::cout << "Parsing client request" << "\n";
			GPR_ASSERT(ok);
			static_cast<CallData*>(tag)->Proceed();
		}
	}

	Store::AsyncService service_;
	std::unique_ptr<ServerCompletionQueue> cq_;
	std::unique_ptr<Server> server_;
	ThreadPool *pool_;
};

std::vector<std::string> getAddresses(std::string addressesLocation) {
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
	ThreadPool *pool = new ThreadPool(nrOfThreads, getAddresses(addressesList));
	StoreImpl store;
	store.RunServer(portNumber, pool);
}
