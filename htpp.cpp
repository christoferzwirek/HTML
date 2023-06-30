//Radosław Wirkus
//wr49442@zut.edu.pl
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

mutex log_mutex;
bool is_running=true;

void run_as_deamon() {
    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
	

    if (pid > 0) {
	ofstream pid_file("deamon.pid");
	if(pid_file.is_open()){
	pid_file<<pid<<endl;
	pid_file.close();
}else{cerr<<"save bload";}
        // Proces macierzysty
        exit(EXIT_SUCCESS);
    }

    umask(0); // Ustawienie maski plików
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

// A helper function to print log messages to the console and write them to the log file
void log(const string& message) {
    time_t t = time(0);
    struct tm* now = localtime(&t);
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", now);
    stringstream ss;
    ss << "[" << buffer << "] " << message << endl;
    string log_message = ss.str();
    lock_guard<mutex> guard(log_mutex);
    cout << log_message;
    ofstream log_file("server.log", ios_base::app);
    if (log_file) {
        log_file << log_message;
    }
}

// A helper function to return the content type of a file based on its extension
string get_content_type(const string& path) {
    if (path.substr(path.find_last_of(".") + 1) == "html") {
        return "text/html";
    } else if (path.substr(path.find_last_of(".") + 1) == "jpg") {
        return "image/jpeg";
    } else if (path.substr(path.find_last_of(".") + 1) == "jpeg") {
        return "image/jpeg";
    } else if (path.substr(path.find_last_of(".") + 1) == "png") {
        return "image/png";
    } else if (path.substr(path.find_last_of(".") + 1) == "gif") {
        return "image/gif";
//      else if (
    } else {
	//return "501";
        return "application/octet-stream";
    }
}
void send_response501(int client_socket, const string& content_type, const string& content) {
    stringstream ss;
    ss << "HTTP/1.1 "<<"501"<<" Not  Implemented \r\n";
  //  ss << "Content-Type: " << content_type << "\r\n";
//    ss << "Content-Length: " << content.length() << "\r\n";
  //  ss << "Connection: close\r\n";
//    ss << "\r\n";
   // ss << content;
    string response = ss.str();
    send(client_socket, response.c_str(), response.length(), 0);
}
// A helper function to send an HTTP response to a client
void send_response(int client_socket, const string& content_type, const string& content) {
    stringstream ss;
    ss << "HTTP/1.1 "<<"200"<<" OK\r\n";
    ss << "Content-Type: " << content_type << "\r\n";
    ss << "Content-Length: " << content.length() << "\r\n";
    ss << "Connection: close\r\n";
    ss << "\r\n";
    ss << content;
    string response = ss.str();
    send(client_socket, response.c_str(), response.length(), 0);
}

// A helper function to send an HTTP error response to a client
void send_error(int client_socket, int status_code, const string& message) {
    stringstream ss;
    ss << "HTTP/1.1 " << status_code << " " << message << "\r\n";
    ss << "Content-Type: text/plain\r\n";
    ss << "Connection: close\r\n";
    ss << "\r\n";
    ss << message;
    string response = ss.str();
    send(client_socket, response.c_str(), response.length(), 0);
}

// A helper function to handle a client request
void handle_request(int client_socket, const string& document_root) {
    // Receive the request from the client
    char buffer[4096];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        log("Error receiving request from client");
        return;
    }
    buffer[bytes_received] = '\0';
    string request = buffer;

    // Parse the request
    string method;
    string path;
    string version;
    istringstream iss(request);
    iss >> method >> path >> version;
// Check if the request is valid
if (method != "GET") {
string content_type = get_content_type(path);
    //send_response501(client_socket, content_type,"string content_type = get_content_type(path);");
    send_error(client_socket, 501, " Not  Implemented");
    log("Invalid request method: " + method);
close(client_socket);
    return;
}

// Check if the requested path is valid
if (path == "/") {
    path = "/index.html";
}
string file_path = document_root + path;
FILE* file = fopen(file_path.c_str(), "rb");
if (!file) {
    send_error(client_socket, 404, " Not Found");
    log("File not found: " + file_path);
close(client_socket);
    return;
}

// Read the file content
string content;
char* buffer2 = new char[1024];
int bytes_read;
while ((bytes_read = fread(buffer2, 1, sizeof(buffer2), file)) > 0) {
    content.append(buffer2, bytes_read);
}
delete[] buffer2;
fclose(file);

// Send the response
string content_type = get_content_type(path);
send_response(client_socket, content_type, content);
log("Served file: " + file_path);

// Close the connection
close(client_socket);
}

void sigint_handler(int signal){
log("Stop server...");

is_running=false;

//execl("./a.out","-s","8080","~/Dokumenty",(char*)NULL);
exit(1);
}

// The main function
int main(int argc, char* argv[]) {
//close(STDIN_FILENO);
//close(STDOUT_FILENO);
//close(STDERR_FILENO);
// Check if the command line arguments are valid
if (argc == 2) {
    if (string(argv[1]) == "-q") {
        ifstream pid_file("deamon.pid");
        if (pid_file.is_open()) {
            string pid_str;
            pid_file >> pid_str;
            pid_file.close();
try{            
int pid = stoi(pid_str);
            kill(pid, SIGTERM);
            cout << "Proces z PID " << pid << " został zatrzymany." << endl;}
catch(const std::invalid_argument& e){
cerr<<"dlad"<<endl;
}
        } else {
            cerr << "Błąd: Plik PID nie został znaleziony." << endl;
        }
        return 0;
    }
}

else if (argc != 4) {
cerr << "Usage: " << argv[0] << "-s <port> <document_root>" << endl;
return 1;
}
run_as_deamon();
/*
ofstream pid_file("deamon.pid");
if(pid_file.is_open()){
	pid_file<<getpid()<<endl;
	pid_file.close();
}else{
cerr<<"blad";
return 0;
}*/

// Parse the command line arguments
int port = atoi(argv[2]);
string document_root = argv[3];

//if(deamon(1,1) <0) {
//	cerr<<"error";
	//return 1;
//}

// Create a socket
int server_socket = socket(AF_INET, SOCK_STREAM, 0);
if (server_socket < 0) {
    cerr << "Error creating socket" << endl;
    return 1;
}

// Bind the socket to the address and port
struct sockaddr_in address;
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(port);
if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
    cerr << "Error binding socket to address and port" << endl;
    return 1;
}

// Listen for incoming connections
if (listen(server_socket, 10) < 0) {
    cerr << "Error listening for incoming connections" << endl;
    return 1;
}

//signal(SIGINT,sigint_handler);

// Start accepting client connections in a loop
while (is_running) {
    // Accept a client connection
    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_size);
    if (client_socket < 0) {
        cerr << "Error accepting client connection" << endl;
        continue;
    }

    // Handle the client request in a separate thread
    thread t(handle_request, client_socket, document_root);
    t.detach();
}

// Close the server socket
close(server_socket);
log("Server stop");

return 0;
}

