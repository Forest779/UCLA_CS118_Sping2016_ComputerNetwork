#ifndef URL_ENCODE_H
#define URL_ENCODE_H
using namespace std;

class Url{
public:
	static string getUrlHost(string url);
	static unsigned getUrlPort(string url);
	static string getUrlPath(string url);
	static string getUrlFileName(string url);
	static string showIP (string hostname);
};

#endif