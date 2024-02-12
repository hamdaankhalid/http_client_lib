#include <__nullptr>
#include <iostream>
#include <string>

#include <http_client.hh>
#include <http_message.hh>

int main() {
  try {
    std::string host =
        "127.0.0.1";      // Specify the hostname or IP address of the server
    int port = 5000;      // Specify the port number
    int blockSize = 1024; // Specify the block size (if needed)

    // Create an HTTPConnection object
    HTTPConnection connection(host, port, blockSize);
    HttpHeader keepAlive("Connection", "Keep-Alive");

    for (int i = 0; i < 5; i++) {
      // Make a GET request to the root path
      std::unique_ptr<HttpResponse> resp =
          connection.Request(HTTP_METHOD::GET, "/tasks", {}, {keepAlive});
      if (resp == nullptr) {
        std::cout << "Failed to make request" << std::endl;
        break;
      }

      std::cout << "----Made Request " << i << " ------" << std::endl;

      std::cout << resp->GetStatusCode() << std::endl;
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
