#include "socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>

using namespace std;

void get_URL(const string &host, const string &path) {
    // Your code here.

    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).

    // 1 connect to http server.
    const Address http_webserver(host, "http");

    // create another socket and connect to the first one
    TCPSocket sock1;

    sock1.connect(http_webserver);
    // 2 send request with url in path.
    sock1.write("GET " + path + " HTTP/1.1\r\n");
    sock1.write("Host: " + host + "\r\n");
    sock1.write("Connection: close\r\n");
    // cout << "we get breakpoints" << endl;
    sock1.write("\r\n");
    // string e = path;
    string recvd;
    // cout << "we get breakpoints" << endl;
    while (!sock1.eof()) {
        recvd = sock1.read();
        cout << recvd;
    }
    //  recvd = sock1.read();
    //     cout << recvd;
    sock1.close();

    // cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
    // cerr << "Warning: get_URL() has not been implemented yet.\n";
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
