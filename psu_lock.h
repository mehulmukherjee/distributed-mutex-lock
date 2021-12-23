//Distributed Mutex Locks based on Ricart-Agrawala algorithm
#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <grpcpp/grpcpp.h>
#include "psu_dml.grpc.pb.h"

#include "psu_node.h"

//gRPC server
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

//gRPC client
using grpc::Channel;
using grpc::ClientContext;

//gRPC PSU_DML
using psu_dml::PSU_DML;
using psu_dml::CSRequest;
using psu_dml::CSReply;

//DML function headers
void psu_init_lock(unsigned int lockno);
void psu_mutex_lock(unsigned int);
void psu_mutex_unlock(unsigned int);

//gRPC function headers
void *runServer(void *arg);
void *runClient(void *arg);
void logDML(std::string, std::string, std::string, unsigned int, int);


//OOP stuff
char Node::nodes[NODES+1][HOSTNAME_LEN];
DMLNode mynode = DMLNode();


//DML stuff
void psu_init_lock(unsigned int lockno) {
	//populate nodes array
	DMLNode::readNodeList();
	//spawn server thread to listen for other node CS request
	pthread_t srvThread;
	DMLNode::args args;
	args.lockno = lockno;
	pthread_create(&srvThread, NULL, &runServer, (void *)&args);
	//pause execution until manual intervention. Useful for setting up n remote hosts
	sleep(1);
//	std::cout << "Press ENTER to continue..." << std::endl;	
//	std::cin.get();
	//TODO implement an auto wait for a response from all other nodes in system...not actually necessary
}

void psu_mutex_lock(unsigned int lockno) {
	//set critical fields for mutex in r&a algorithm
	mynode.requesting_cs[lockno] = true;
	mynode.myseqno = mynode.highestseqno + 1;	
	//send CS request to other nodes, then wait until all ACKs are received
	pthread_t cliThreads[NODES];
	DMLNode::args args[NODES];
	int my_t = -1;
	for(int t = 0; t < NODES; t++) {
		if (!mynode.hostname.compare(DMLNode::nodes[t])) { my_t = t; continue; }
		args[t].lockno = lockno;	
		strncpy(args[t].hostname, DMLNode::nodes[t], HOSTNAME_LEN);
		pthread_create(&cliThreads[t], NULL, &runClient, (void *)&args[t]);
	}
	for(int t = 0; t < NODES; t++) {
		if(t == my_t) continue;
		pthread_join(cliThreads[t], NULL);
	}
}

void psu_mutex_unlock(unsigned int lockno) {
	mynode.requesting_cs[lockno] = false;
	/*
 	* Deferred ACKs are handled by our gRPC server implementation...
 	* 
 	* There is a busy wait on the requesting_cs bool flag in the server.
 	* Those busy waits will release when requesting_cs is set to false (as
 	* seen above), so the server will finish serving its request.
 	*
 	* gRPC supposedly queue's requests sequentially, so there shouldn't
 	* be a problem with sequential consistency.
 	*/
}


//gRPC stuff
//gRPC server/receiver stuff
class PSU_DML_Implementation final : public PSU_DML::Service {
public:	
	unsigned int srv_lockno = -1;	

	//implement requestCS() service
	Status requestCS(ServerContext* context, const CSRequest* request, CSReply* reply) override {
		//Request is received from node that wants CS...
		logDML(mynode.hostname, request->host(), "requestCS", request->lockno(), request->seqno());
		reply->set_host(mynode.hostname);
		if(request->lockno() != srv_lockno) {
			reply->set_msg("error: [request_lockno != server_lockno]");
			return Status::OK;
		}
		//check if node should send ACK. busy wait to deferr ACK. tie breaker is hostname string
		mynode.highestseqno = std::max(mynode.highestseqno, request->seqno());
		if(mynode.requesting_cs[request->lockno()] && mynode.myseqno < request->seqno()) {
			while(mynode.requesting_cs[request->lockno()]);
		} else if(mynode.requesting_cs[request->lockno()] && mynode.myseqno == request->seqno()
				&& request->host() < reply->host()){
			while(mynode.requesting_cs[request->lockno()]);
		}
		reply->set_msg("[" + reply->host() + "] ACK RECEIVED");
		return Status::OK;
	}
};

void *runServer(void *arg) {	
	//setting params	
	DMLNode::args *args = (DMLNode::args *) arg;
	std::string address(mynode.ip_addr + ":" + DMLNode::generatePort(args->lockno));
	
	//initializing service
	PSU_DML_Implementation service;
	service.srv_lockno = args->lockno;
	ServerBuilder builder;

	//standard service execution code
	builder.AddListeningPort(address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());
//	std::cout << "Server listening on port: " << address << std::endl;

	server->Wait();
}


//gRPC client/requestor stuff
class PSU_DML_Client {
public:
	
	PSU_DML_Client(std::shared_ptr<Channel> channel) : stub_(PSU_DML::NewStub(channel)) {}

	std::string sendRequest(std::string target_hostname, unsigned int lockno) {
		CSRequest request;
		request.set_host(mynode.hostname);
		request.set_seqno(mynode.myseqno);
		request.set_lockno(lockno);
		CSReply reply;
		ClientContext context;
		Status status = stub_->requestCS(&context, request, &reply);

		if(status.ok()){
			return reply.msg();
		} else {
			//std::cout << status.error_code() << ": " << status.error_message() << std::endl;
			return "[" + target_hostname + "] not running...";
		}
	}

private:
	std::unique_ptr<PSU_DML::Stub> stub_;
};

void *runClient(void *arg) {
	//setting params
	DMLNode::args *args = (DMLNode::args *) arg;
	std::string address(DMLNode::getIPAddress((char *) args->hostname) + ":" + DMLNode::generatePort(args->lockno));
	
	//initializing client
	PSU_DML_Client client(
		grpc::CreateChannel(address, grpc::InsecureChannelCredentials())	
	);

	//executing client request and awaiting reply
	std::string reply;
	reply = client.sendRequest(std::string((char *) args->hostname), args->lockno);
	//std::cout << reply << std::endl;
}

