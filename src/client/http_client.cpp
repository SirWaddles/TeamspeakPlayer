#include "http_client.h"
#include <cstdlib>
#include <boost/network.hpp>

HTTPClient::~HTTPClient(){

}


class HTTPClientImpl : public HTTPClient {
public:
	HTTPClientImpl();
	virtual ~HTTPClientImpl();

	virtual void WriteToStream(std::ofstream& writer);
	virtual std::string WriteToString();

	virtual void RunRequest(std::string url);
	virtual void RunHeaders();
	virtual void RunBody();

	virtual int GetContentSize();
private:

	boost::network::http::client client;
	boost::network::http::client::request request;
	boost::network::http::client::response response;

};


HTTPClientImpl::HTTPClientImpl(){

}

HTTPClientImpl::~HTTPClientImpl(){

}

void HTTPClientImpl::RunRequest(std::string url){
	request = boost::network::http::client::request(url);
	request << boost::network::header("Connection", "close");
}

void HTTPClientImpl::RunHeaders(){
	response = client.head(request);
}

void HTTPClientImpl::RunBody(){
	response = client.get(request);
}

int HTTPClientImpl::GetContentSize(){
	auto headers = boost::network::http::headers(response);
	for (auto it = headers.begin(); it != headers.end(); ++it){
		if (it->first == "Content-length"){
			return std::stoi(it->second);
		}
	}
	return 0;
}

void HTTPClientImpl::WriteToStream(std::ofstream& writer){
	writer << boost::network::http::body(response);
}

std::string HTTPClientImpl::WriteToString(){
	return response.body();
}

HTTPClient* GetNewClient(){
	return new HTTPClientImpl();
}