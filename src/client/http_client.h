#ifndef __HTTP_CLIENT__
#define __HTTP_CLIENT__

#include <fstream>
#include <string>

class HTTPClient {
public:
	virtual ~HTTPClient();

	virtual void WriteToStream(std::ofstream& writer) = 0;
	virtual std::string WriteToString() = 0;

	virtual void RunRequest(std::string url) = 0;

	virtual void RunHeaders() = 0;
	virtual void RunBody() = 0;
	virtual int GetContentSize() = 0;
private:

};

HTTPClient* GetNewClient();

#endif // __HTTP_CLIENT__