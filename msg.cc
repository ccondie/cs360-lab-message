#include "msg.h"

Msg::Msg(string host, int port, bool debug_t) 
{
    // setup variables
    host_ = host;
    port_ = port;
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
    debugFlag = debug_t;
}

Msg::~Msg() {
}

void Msg::run() 
{
    debug("Msg::run()");
    // connect to the server and run echo program
    create();
    // echo();
    message();
}

void
Msg::create() 
{
    debug("Msg::create()");
    struct sockaddr_in server_addr;

    // use DNS to get IP address
    struct hostent *hostEntry;
    hostEntry = gethostbyname(host_.c_str());
    if (!hostEntry) {
        cout << "No such host name: " << host_ << endl;
        exit(-1);
    }

    // setup socket address structure
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    memcpy(&server_addr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);

    // create socket
    server_ = socket(PF_INET,SOCK_STREAM,0);
    if (!server_) {
        perror("socket");
        exit(-1);
    }

    // connect to server
    if (connect(server_,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("connect");
        exit(-1);
    }
}

void
Msg::close_socket() 
{
    close(server_);
}

void
Msg::echo() 
{
    string line;
    
    // loop to handle user interface
    while (getline(cin,line)) 
    {
        // append a newline
        line += "\n";
        // send request
        bool success = send_request(line);
        // break if an error occurred
        if (not success)
            break;
        // get a response
        success = get_response();
        // break if an error occurred
        if (not success)
            break;
    }
    close_socket();
}

void
Msg::message()
{
    debug("Msg::message()");

    stringstream ss;

    string line;
    string command;

    while(1)
    {
        cout << "% ";

        line = "";
        ss.str("");

        getline(cin, line);
        line += "\n";

        ss << line;
        if(ss.str().length() > 0)
            ss >> command;

        bool commandFound = false;

        debug("Msg::message()::ss.str():" + ss.str());
        debug("Msg::message()::command:" + command);

        if(command.length() > 0)
        {
            if(command == "send")
            {
                debug("Msg::message()::send:: command: " + command);
                msg_send(line);
                commandFound = true;
            }

            if(command == "list")
            {
                debug("Msg::message()::list:: command: " + command);
                msg_list(line);
                commandFound = true;
            }    

            if(command == "read")
            {
                debug("Msg::message()::read:: command: " + command);
                msg_read(line);
                commandFound = true;
            }

            if(command == "reset")
            {
                debug("Msg::message()::reset:: command: " + command);
                msg_reset();
                commandFound = true;
            }

            if(command == "quit")
            {
                debug("Msg::message()::quit:: command: " + command);
                commandFound = true;
                exit(0);

            }

            if(!commandFound)
                cout << "Command not found: " << command << endl;
        }
    }
    close_socket();
}

void
Msg::msg_send(string line)
{
    stringstream ss;
    ss << line;

    string command;
    string user;
    string subject;

    string lineTest;
    string message;

    stringstream protocol;

    int exitState = 0;

    debug("Msg::msg_send()::inputLine: " + line);

    getline(ss, command, ' ');
    getline(ss, user, ' ');
    getline(ss, subject, '\n');

    debug("Msg::msg_send():: command: " + command);
    debug("Msg::msg_send():: user: " + user);
    debug("Msg::msg_send():: subject: " + subject);

    cout << "- Type your message. End with a blank line -" << endl;

    while(exitState != 1)
    {
        getline(cin, lineTest);

        if(lineTest.length() == 0)
        {
            exitState = exitState + 1; 
        }   
        else
        {
            message.append(lineTest + "\n");
            debug("Msg::msg_send(): lineTest: " + lineTest);
        }   
    }

    protocol << "put " << user << " " << subject << " " << message.length() << "\n" << message;

    debug("Msg::msg_send(): send_request(): protocol: " + protocol.str());
    bool success = send_request(protocol.str());

    if (not success)
        cout << "SOMETHING WHEN WRONG send_request" << endl;
    // get a response
    success = get_response();
    // break if an error occurred
    if (not success)
        cout << "SOMETHING WENT WRONG IN get_response" << endl;
}

void
Msg::msg_list(string line)
{
    debug("Msg::msg_list()::line: " + line);

    stringstream ss;
    ss << line;

    string command;
    string user;

    ss >> command;
    ss >> user;

    debug("Msg::msg_list():: command: " + command);
    debug("Msg::msg_list():: user: " + user);

    stringstream protocol;
    protocol << "list " << user << "\n";

    debug("Msg::msg_list():: send_request():: protocol: " + protocol.str());
    bool success = send_request(protocol.str());
    

    if (not success)
        cout << "SOMETHING WHEN WRONG send_request" << endl;
    // get a response
    success = get_response();
    // break if an error occurred
    if (not success)
        cout << "SOMETHING WENT WRONG IN get_response" << endl;
}

void
Msg::msg_read(string line)
{
    debug("Msg::msg_read()::line: " + line);

    stringstream ss;
    ss << line;

    string command;
    string user;
    string index;

    ss >> command;
    ss >> user;
    ss >> index;

    debug("Msg::msg_read():: command: " + command);
    debug("Msg::msg_read():: user: " + user);
    debug("Msg::msg_read():: index: " + index);

    stringstream protocol;
    protocol << "get " << user << " " << index << "\n";

    debug("Msg::msg_read():: send_request():: protocol: " + protocol.str());
    bool success = send_request(protocol.str());
    

    if (not success)
        cout << "SOMETHING WHEN WRONG send_request" << endl;
    // get a response
    success = get_response();
    // break if an error occurred
    if (not success)
        cout << "SOMETHING WENT WRONG IN get_response" << endl;
}

void
Msg::msg_reset()
{
    debug("Msg::msg_reset()");
    bool sucess = send_request("reset");
}

bool
Msg::send_request(string request) {
    // prepare to send request
    const char* ptr = request.c_str();
    int nleft = request.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(server_, ptr, nleft, 0)) < 0) {
            if (errno == EINTR) {
                // the socket call was interrupted -- try again
                continue;
            } else {
                // an error occurred, so break out
                perror("write");
                return false;
            }
        } else if (nwritten == 0) {
            // the socket is closed
            return false;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return true;
}

bool
Msg::get_response() {
    string response = "";
    // read until we get a newline
    while (response.find("\n") == string::npos) {
        int nread = recv(server_,buf_,1024,0);
        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                return "";
        } else if (nread == 0) {
            // the socket is closed
            return "";
        }
        // be sure to use append in case we have binary data
        response.append(buf_,nread);
    }
    // a better Msg would cut off anything after the newline and
    // save it in a cache
    cout << response;
    return true;
}

void
Msg::debug(string message)
{
    if(debugFlag)
        cout << "\t" << message << endl;
}