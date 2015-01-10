#ifndef __HTTP_CLIENT_HEADER__
#define __HTTP_CLIENT_HEADER__

#include <boost/asio.hpp>
#include <string>

struct URLDetails{
	std::string access;
	std::string server;
	std::string query;
};

class client {
public:
	client(boost::asio::io_service& io_service, const std::string& server, const std::string& path);
	std::string* GetRequestData();
	static URLDetails SplitURL(std::string url);
private:
	void handle_resolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void handle_connect(const boost::system::error_code& err);
	void handle_write_request(const boost::system::error_code& err);
	void handle_read_status_line(const boost::system::error_code& err);
	void handle_read_headers(const boost::system::error_code& err);
	void handle_read_content(const boost::system::error_code& err);

	boost::asio::ip::tcp::resolver resolver_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf request_;
	boost::asio::streambuf response_;

	std::string httpData;
};



#endif // __HTTP_CLIENT_HEADER__