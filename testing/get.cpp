#include <iostream>
#include <string>

#include <http_client.hh>

int main() 
{
    try {
        std::string host = "127.0.0.1"; // Specify the hostname or IP address of the server
        int port = 5000; // Specify the port number
        int blockSize = 1024; // Specify the block size (if needed)

        // Create an HTTPConnection object
        HTTPConnection connection(host, port, blockSize);
		
		HttpHeader foo("Foo", "bar");
		HttpHeader keepAlive("Connection", "keep-alive");

		for (int i = 0; i < 1; i++)
			// Make a GET request to the root path
			connection.Request(HTTP_METHOD::GET, "/tasks", {}, {keepAlive, foo}); 

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
