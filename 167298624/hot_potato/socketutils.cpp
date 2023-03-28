#include "socketutils.h"


void print_statement(const std::string & message){
    std::cout << message << std::endl;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////          Set-up/Connection            /////////////////////////////////////////////

void err_msg_routine(const std::string & initial_msg, const char *host_name, const std::string & port_num){
    std::stringstream err_msg;
    err_msg << initial_msg << std::endl;
    err_msg << "  (" << host_name << "," << port_num << ")" << std::endl;
    std::cerr << err_msg.str();
    exit(1);
}

void set_up_ringmaster_addrinfo(struct addrinfo *hints){
    memset(&(*hints), 0, sizeof (*hints)); // make sure the hints struct is empty

    hints->ai_family = AF_UNSPEC;     // Don't care if the socket is IPv4 or IPv6
    hints->ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints->ai_flags = AI_PASSIVE;     // Fill in my IP for me. This tells the program to bind the to the IP the host is running on
}

void set_up_player_addrinfo(struct addrinfo *hints){
    memset(&(*hints), 0, sizeof (*hints)); // make sure the hints struct is empty

    hints->ai_family = AF_UNSPEC;     // Don't care if the socket is IPv4 or IPv6
    hints->ai_socktype = SOCK_STREAM; // TCP stream sockets
}


void call_getaddrinfo(struct addrinfo *hints, struct addrinfo **servinfo, const char *host_name, const std::string & port_num){
    int status = getaddrinfo(host_name, port_num.c_str(), hints, &*servinfo);
    if (status != 0) { //Notify user of listen errors on socketfd
        std::stringstream initial_msg;
        initial_msg << "getaddrinfo error" << gai_strerror(status) << std::endl;
        err_msg_routine(initial_msg.str(), host_name, port_num);
    }
}

int build_socket_fd(struct addrinfo *servinfo, const char *host_name, const std::string & port_num){
    // build the socket file descriptor
    int socket_file_descriptor;
    socket_file_descriptor = socket(servinfo->ai_family,
                                    servinfo->ai_socktype,
                                    servinfo->ai_protocol);

    if (socket_file_descriptor == -1) { //Notify user of listen errors on socketfd
        std::string initial_msg =  "Error: cannot create socket" ;
        err_msg_routine(initial_msg, host_name, port_num);
    }

    return socket_file_descriptor;
}

void bind_socket(int socket_file_descriptor, struct addrinfo *servinfo, const char *host_name, const std::string & port_num){
    int status = bind(socket_file_descriptor, servinfo->ai_addr, servinfo->ai_addrlen);
    if (status == -1) { //Notify user of listen errors on socketfd
        std::string initial_msg = "Error: cannot bind socket";
        err_msg_routine(initial_msg, host_name, port_num);
    }
}

void connect_to_socket(int socket_file_descriptor, struct addrinfo *servinfo, const char *host_name, const std::string & port_num){
    int status = connect(socket_file_descriptor,
                         servinfo->ai_addr,
                         servinfo->ai_addrlen); /*  Open a connection on socket FD to peer at ADDR (which LEN bytes long).
                                                        For connectionless socket types, just set the default address to send to
                                                        and the only address from which to accept transmissions.
                                                        Return 0 on success, -1 for errors.*/

    if (status == -1) { //Notify user of listen errors on socketfd
        std::string initial_msg = "Error: cannot connect to socket";
        err_msg_routine(initial_msg, host_name, port_num);
    }
}

std::pair<int, std::string> listen_on_socket_players(int socket_file_descriptor, const char *host_name, const std::string & port_num, int n, int server_fd){
    int status = listen(socket_file_descriptor, n); /* Prepare to accept connections on socket FD.
                                             N connection requests will be queued before further requests are refused.
                                             Returns 0 on success, -1 for errors.  */

    if (status == -1) { //Notify user of listen errors on socketfd
        std::string initial_msg = "Error: cannot listen on socket";
        err_msg_routine(initial_msg, host_name, port_num);
    }



    struct sockaddr_in socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;

    send_ACK(server_fd);
    client_connection_fd = accept(socket_file_descriptor, (struct sockaddr *)&socket_addr, &socket_addr_len); /* Await a connection on socket FD.
                                                                                                                                  When a connection arrives, open a new socket to communicate with it,
                                                                                                                                  set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
                                                                                                                                  peer and *ADDR_LEN to the address's actual length, and return the
                                                                                                                                  new socket's descriptor, or -1 for errors.*/
    if (client_connection_fd == -1) { // Notify user of socket acceptance errors on socketfd
        std::string initial_msg = "Error: cannot accept connection on socket";
        err_msg_routine(initial_msg, host_name, port_num);
    }

    std::stringstream ss;
    ss << inet_ntoa(socket_addr.sin_addr) << "|" << (int) ntohs(socket_addr.sin_port);
    std::string ip_and_port = ss.str();
    std::pair<int, std::string> client_con_info;
    client_con_info.first = client_connection_fd;
    client_con_info.second = ip_and_port;

    send_ACK(client_connection_fd);
    //wait_for_ACK(client_connection_fd);

    return client_con_info;
}


std::vector<std::pair<int, std::string> > listen_on_socket_for_all_players(int socket_file_descriptor, const char *host_name, const std::string & port_num, size_t n){
    int status = listen(socket_file_descriptor, (int) n); /* Prepare to accept connections on socket FD.
                                             N connection requests will be queued before further requests are refused.
                                             Returns 0 on success, -1 for errors.  */

    if (status == -1) { //Notify user of listen errors on socketfd
        std::string initial_msg = "Error: cannot listen on socket";
        err_msg_routine(initial_msg, host_name, port_num);
    }

    std::vector<std::pair<int, std::string> > client_con_info;
    do{

        struct sockaddr_in socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        int client_connection_fd;
        client_connection_fd = accept(socket_file_descriptor, (struct sockaddr *)&socket_addr, &socket_addr_len); /* Await a connection on socket FD.
                                                                                                                       When a connection arrives, open a new socket to communicate with it,
                                                                                                                       set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
                                                                                                                       peer and *ADDR_LEN to the address's actual length, and return the
                                                                                                                       new socket's descriptor, or -1 for errors.*/
        if (client_connection_fd == -1) { // Notify user of socket acceptance errors on socketfd
            std::string initial_msg = "Error: cannot accept connection on socket";
            err_msg_routine(initial_msg, host_name, port_num);
        }

        std::stringstream ss;
        ss << inet_ntoa(socket_addr.sin_addr) << "|" << (int) ntohs(socket_addr.sin_port);
        std::string ip_and_port = ss.str();
        std::pair<int, std::string> fd_idport_pair;
        fd_idport_pair.first = client_connection_fd;
        fd_idport_pair.second = ip_and_port;
        client_con_info.push_back(fd_idport_pair);

        send_ACK(client_connection_fd);
        wait_for_ACK(client_connection_fd);
    }while(client_con_info.size()<n);
    return client_con_info;
}

int get_random_player(const std::string & num_players, const int & player_id){
    int random_player;
    do{
        random_player  = rand() % str_to_integer(num_players);
    }while(random_player == player_id);

    return random_player;
}

std::string random_get_left_or_right(){
    int rn = get_random_player("2", 3);
    if(rn==0){
        return "left";
    }
    return "right";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Wait for ACK

void wait_for_ACK(int fd){
    char buffer[2048];
    memset(&(buffer), 0, sizeof (buffer)); // make sure the hints struct is empty
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 1;
    bool found_ACK = false;
    do{
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        select(fd+1, &readfds, NULL , NULL, &tv);
        if(FD_ISSET(fd, &readfds)){
            recv(fd, buffer, sizeof buffer, 0);
            std::string message(buffer);
            if(message == "ACK"){
                found_ACK = true;
            }else{
                std::cerr << "Received non ACK message while waiting for ACK: " << message << std::endl;
            }
        }
    }while(!found_ACK);
}

void wait_for_multiple_ACK(std::vector<int> listen_client_con_fds){

    std::vector<std::pair<bool, int> > pid_fd_map;
    for(size_t i = 0; i < listen_client_con_fds.size(); i++){
        std::pair<bool, int> temp;
        temp.first = false; // message has not been sent yet
        temp.second = listen_client_con_fds[i];
        pid_fd_map.push_back(temp);
    }

    // use select to monitor sockets
    fd_set readfds;
    char buffer[2048];
    memset(&(buffer), 0, sizeof (buffer)); // make sure the hints struct is empty
    struct timeval tv;
    tv.tv_sec = 1;

    bool all_fds_recieved_message = true;
    do {

        // set up the sockets we plan to write too
        FD_ZERO(&readfds);
        int n = INT_MIN;
        for (size_t i = 0; i < listen_client_con_fds.size(); i++) {
            if (!pid_fd_map[i].first) {
                FD_SET(pid_fd_map[i].second, &readfds);
                if (pid_fd_map[i].second > n) {
                    n = pid_fd_map[i].second;
                }
            }
        }

        // send message to remaining Players
        int rv = select(n+1, &readfds, NULL, NULL, &tv);
        if(rv == -1){
            std::cerr << "Select() error occurred" << std::endl;
        }else if(rv == 0){
            //std::cerr << "Timeout occurred! No data after 1 seconds" << std::endl;
        }else{
            for(size_t i = 0; i < listen_client_con_fds.size(); i++){
                if(FD_ISSET(listen_client_con_fds[i], &readfds)){
                    memset(&(buffer), 0, sizeof (buffer)); // make sure the hints struct is empty
                    recv(listen_client_con_fds[i], buffer, sizeof buffer, 0);
                    std::string message(buffer);
                    if(message == "ACK"){
                        pid_fd_map[i].first = true;                                     // ACK recieved
                    }else{
                        std::cerr << "   Received non ACK message while waiting for ACK" << std::endl;
                    }
                }
            }
        }

        // check to see if we can break the loop
        all_fds_recieved_message = true;
        for(size_t i = 0; i < listen_client_con_fds.size(); i++){
            if(!pid_fd_map[i].first){
                all_fds_recieved_message = false;
            }
        }
    }while(!all_fds_recieved_message);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// SEND MESSAGE

/**
 * This method does not use select
 * @param msg
 */
void send_message(int fd, const std::string & msg){
    const char *message = msg.c_str();
    send(fd, message, strlen(message), 0);
}

void send_potato(int fd, const Potato & tater){
    std::string potato_string = pack_potato(tater);
    send_message(fd, potato_string);
    wait_for_ACK(fd);
    send_ACK(fd);
}


void send_ACK(int fd){
    std::string msg = "ACK";
    send_message(fd, msg);
}

void select_send_messages_to_multiple_fds(std::vector<std::string> message_vector, std::vector<int> listen_client_con_fds){

    std::vector<std::pair<bool, int> > pid_fd_map;
    for(size_t i = 0; i < listen_client_con_fds.size(); i++){
        std::pair<bool, int> temp;
        temp.first = false; // message has not been sent yet
        temp.second = listen_client_con_fds[i];
        pid_fd_map.push_back(temp);
    }

    fd_set writefds;
    struct timeval tv;
    tv.tv_sec = 1;

    bool all_fds_sent_message = true;
    do{

        // set up the sockets we plan to write too
        FD_ZERO(&writefds);
        int n = INT_MIN;
        for(size_t i = 0; i < listen_client_con_fds.size(); i++){
            if(!pid_fd_map[i].first){
                FD_SET(pid_fd_map[i].second, &writefds);
                if(pid_fd_map[i].second > n){
                    n = pid_fd_map[i].second;
                }
            }
        }

        // send message to remaining Players
        int rv = select(n+1, NULL, &writefds, NULL, &tv);
        if(rv == -1){
            std::cerr << "Select() error occurred" << std::endl;
        }else if(rv == 0){
            //std::cerr << "Timeout occurred! No data after 1 seconds" << std::endl;
        }else{
            for(size_t i = 0; i < listen_client_con_fds.size(); i++){
                if(FD_ISSET(listen_client_con_fds[i], &writefds)){
                    const char *message = message_vector[i].c_str();
                    send(listen_client_con_fds[i], message, strlen(message), 0);
                    pid_fd_map[i].first = true;                                     // message sent now
                }
            }
        }

        // check to see if we can break the loop
        all_fds_sent_message = true;
        for(size_t i = 0; i < listen_client_con_fds.size(); i++){
            if(!pid_fd_map[i].first){
                all_fds_sent_message = false;
            }
        }
    }while(!all_fds_sent_message);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// RECIEVE MESSAGE

std::string recieve(int fd){
    char buffer[2048]; // ensures plenty of space for a long potato trace string
    memset(&(buffer), 0, sizeof (buffer)); // make sure the hints struct is empty
    recv(fd, buffer, 256, 0);
    std::string message(buffer);
    return message;
}

///////////////////////////////// WAIT FOR MESSAGE

std::vector<std::pair<int, std::string> >  wait_for_any_messages(std::vector<int> listen_client_con_fds){
    // use select to monitor sockerts
    fd_set readfds;
    char buffer[2048];
    memset(&(buffer), 0, sizeof (buffer)); // make sure the hints struct is empty
    //struct timeval tv;
    //tv.tv_sec = 1;

    FD_ZERO(&readfds); // clear the set ahead of time
    int n = INT_MIN;
    for(size_t i = 0; i < listen_client_con_fds.size(); i++){
        FD_SET(listen_client_con_fds[i], &readfds); // add our descriptors to the set
        if(listen_client_con_fds[i] > n){
            n = listen_client_con_fds[i];
        }
    }

    std::vector<std::pair<int, std::string> > message_vector;

    bool legitimate_message_recieved = false;
    do{
        int rv = select(n+1, &readfds, NULL, NULL, NULL); // timeout = NULL means wait until a message shows up
        if(rv == -1){
            std::cerr << "Select() error occurred" << std::endl;
        }else if(rv == 0){
            //std::cerr << "Timeout occurred! No data after 10 seconds" << std::endl;
        }else{
            for(size_t i = 0; i < listen_client_con_fds.size(); i++){
                if(FD_ISSET(listen_client_con_fds[i], &readfds)){

                    memset(&(buffer), 0, sizeof (buffer)); // make sure the hints struct is empty
                    recv(listen_client_con_fds[i], buffer, sizeof buffer, 0);
                    std::string msg(buffer);

                    if(!msg.empty()){
                        //sometimes random emtpy messages are recieved, this check prevents them from being accepted
                        std::pair<int, std::string> message_pair;
                        message_pair.first = i;
                        message_pair.second = msg;
                        message_vector.push_back(message_pair);
                        legitimate_message_recieved=true;
                    }
                }
            }
        }
    }while(!legitimate_message_recieved);

    return message_vector;
}