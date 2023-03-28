#include <cstring>      //memset
#include <cstdlib>
#include <cstdio>
#include <iostream>     //std::cout
#include <sstream>
#include <sys/socket.h> //socket
#include <netdb.h>      //addrinfo
#include <vector>
#include <assert.h>
#include <limits>
#include <arpa/inet.h>
#include <bits/stdc++.h>

#include "potato.hpp"

int str_to_integer(std::string number_string);

class PlayerInfo{
private:
    public:
    int p_id;
    std::string host_name;
    std::string sock_name;
    std::string packed_string;

    std::string left_nbr_packed_string;
    std::string right_nbr_packed_string;

    int fd;

    PlayerInfo() : p_id(-1) {}

    PlayerInfo(int player_id, std::string host_nm, std::string sock_nm) : p_id(player_id), host_name(host_nm), sock_name(sock_nm), fd(-1) {
        build_packed_string();
    }

    explicit PlayerInfo(std::string packed_str) : packed_string(packed_str){
        unpack_packed_string();
    }

    PlayerInfo & operator=(const PlayerInfo & rhs){
        if(this != &rhs){
            this->p_id = rhs.p_id;
            this->host_name = rhs.host_name;
            this->sock_name = rhs.sock_name;
            this->packed_string = rhs.packed_string;
            this->left_nbr_packed_string = rhs.left_nbr_packed_string;;
            this->right_nbr_packed_string = rhs.right_nbr_packed_string;;
            this->fd = rhs.fd;
        }
        return *this;
    }

    void update_pid(int new_p_id){
        this->p_id = new_p_id;
        build_packed_string();
    }

    void update_host_name(const std::string & new_host_name){
        this->host_name = new_host_name;
        build_packed_string();
    }

    void update_sock_name(const std::string & new_sock_name){
        this->sock_name = new_sock_name;
        build_packed_string();
    }

    void update_left_nbr(const std::string & left_nbr){
        this->left_nbr_packed_string = left_nbr;
    }

    void update_right_nbr(const std::string & right_nbr){
        this->right_nbr_packed_string = right_nbr;
    }

    void update_nbrs(const std::string & left_nbr, const std::string & right_nbr){
        update_left_nbr(left_nbr);
        update_right_nbr(right_nbr);
    }

    std::string get_nbr_message_string(){
        std::stringstream ss;
        ss << this->left_nbr_packed_string << this->right_nbr_packed_string;
        std::string neighboor_string = ss.str();
        return neighboor_string;
    }

    void build_packed_string(){
        std::stringstream ss;
        ss << "#" << this->p_id << "|" << this->host_name << "|" << this->sock_name;
        this->packed_string = ss.str();
    }

    void unpack_packed_string(){
        std::string PlayerInfoFlag = this->packed_string.substr(0,1);
        bool PlayerInfoFlagFound = false;

        if(PlayerInfoFlag == "#"){
            PlayerInfoFlagFound = true;
        }

        if(PlayerInfoFlagFound){
            std::string packed_string_no_flag = this->packed_string.substr(1,this->packed_string.size());

            // get player ID
            int pid_split = packed_string_no_flag.find("|");
            std::string pid = packed_string_no_flag.substr(0, pid_split);
            this->p_id = str_to_integer(pid);

            // host name
            std::string packed_string_no_flag_no_pid = packed_string_no_flag.substr(pid_split + 1,packed_string_no_flag.size());
            int host_name_split = packed_string_no_flag_no_pid.find("|");
            std::string host_nm = packed_string_no_flag_no_pid.substr(0, host_name_split);
            this->host_name = host_nm;

            // sock name
            std::string packed_string_no_flag_no_pid_no_hostnm = packed_string_no_flag_no_pid.substr(host_name_split + 1, packed_string_no_flag_no_pid.size());
            this->sock_name = packed_string_no_flag_no_pid_no_hostnm;

        }else{
            std::cerr << "PlayerInfor error. # flag not found in passed packed string" << std::endl;
        }
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////          Set-up/Connection            /////////////////////////////////////////////

void err_msg_routine(const std::string & initial_msg, const char *host_name, const std::string & port_num);

void set_up_ringmaster_addrinfo(struct addrinfo *hints);

void set_up_player_addrinfo(struct addrinfo *hints);

void call_getaddrinfo(struct addrinfo *hints, struct addrinfo **servinfo, const char *host_name, const std::string & port_num);

int build_socket_fd(struct addrinfo *servinfo, const char *host_name, const std::string & port_num);

void bind_socket(int socket_file_descriptor, struct addrinfo *servinfo, const char *host_name, const std::string & port_num);


std::pair<int, std::string> listen_on_socket_players(int socket_file_descriptor, const char *host_name, const std::string & port_num, int n, int server_fd);
std::vector<std::pair<int, std::string> > listen_on_socket_for_all_players(int socket_file_descriptor, const char *host_name, const std::string & port_num, size_t n);

void connect_to_socket(int socket_file_descriptor, struct addrinfo *servinfo, const char *host_name, const std::string & port_num);

int get_random_player(const std::string & num_players, const int & player_id);

std::string random_get_left_or_right();
////
//// SEND Message
////
void send_message(int fd, const std::string & msg);
void send_potato(int fd, const Potato & tater);
void send_ACK(int fd);

//void select_send_message(int fd, const std::string& msg);
void select_send_messages_to_multiple_fds(std::vector<std::string> message_vector, std::vector<int> listen_client_con_fds);

////
//// RECEIVE Message
////
std::string recieve(int fd);
void print_statement(const std::string & message);
void print_statement_stdcerr(const std::string & message);
void wait_for_ACK(int fd);
std::vector<std::pair<int, std::string> >  wait_for_any_messages(std::vector<int> listen_client_con_fds);
void wait_for_multiple_ACK(std::vector<int> listen_client_con_fds);

