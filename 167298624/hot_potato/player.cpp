#include <cstring>
#include <vector>
#include <string>
#include <iostream>
#include <netdb.h>      // addrinfo
#include "socketutils.h"
#include <unistd.h>     //close
#include <cstdlib>
#include <netinet/in.h>

class Player{
private:
    // Initial Args
    std::string machine_name;
    std::string port_num_server;

    // Server connection socket
    struct addrinfo hints_server;
    struct addrinfo *servinfo_server;
    const char *host_name_server;
    int server_fd;

    // Player listen socket
    struct addrinfo hints_left_nbr;
    struct addrinfo *servinfo_left_nbr; // point to the results
    const char *host_name_left_nbr;
    int left_nbr_fd;
    std::string port_num_left;
    int player_fd;

    // Right neighbor connect
    struct addrinfo hints_right_nbr;
    struct addrinfo *servinfo_right_nbr;
    const char *host_name_right_nbr;
    int right_nbr_fd;
    std::string port_num_right;

    // Player id
    int total_players;
    std::vector<int> fd_vect;

    // Potato
    Potato hot_potato;

    // PlayerInfo
    PlayerInfo pi;
    std::vector<PlayerInfo> pi_arr;

    // Connection tracker
    bool left_closed;
    bool right_closed;
    bool server_closed;
    bool listener_only;
    bool connector_only;

    void set_up_send_socket(){
        host_name_server = machine_name.c_str();
        set_up_player_addrinfo(&hints_server);
        call_getaddrinfo(&hints_server,
                         &servinfo_server,
                         host_name_server,
                         port_num_server.c_str());

        this->server_fd = build_socket_fd(servinfo_server,
                                          host_name_server,
                                          port_num_server);

        connect_to_socket(this->server_fd,
                          servinfo_server,
                          host_name_server,
                          port_num_server);
        wait_for_ACK(this->server_fd);
        send_ACK(this->server_fd);
        this->fd_vect.push_back(this->server_fd);
    }

    void build_listen_socket(){
        port_num_left = "0";
        std::string ip = "0.0.0.0";
        host_name_left_nbr = ip.c_str();
        set_up_ringmaster_addrinfo(&hints_left_nbr);

        call_getaddrinfo(&hints_left_nbr,
                         &servinfo_left_nbr,
                         host_name_left_nbr,
                         port_num_left);

        this->player_fd = build_socket_fd(servinfo_left_nbr,
                                          host_name_left_nbr,
                                          port_num_left);                                                // 3. Build the socket


        bind_socket(player_fd,
                    servinfo_left_nbr,
                    host_name_left_nbr,
                    port_num_left);

        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        int status = getsockname(player_fd, (struct sockaddr *)&addr, &addr_len);
        if(status != 0){
            std::cerr << "Error getsockname()" << std::endl;
            exit(1);
        }
        int port_number = (int) ntohs(addr.sin_port);
        std::stringstream ss;
        ss << port_number;
        std::string pnl = ss.str();
        port_num_left = ss.str();
    }

    void send_player_listen_port_to_server(){

        wait_for_ACK(this->server_fd);
        send_message(this->server_fd, port_num_left);
        wait_for_ACK(this->server_fd);
        send_ACK(this->server_fd);
    }

    void unpack_id_string(){
        std::string id_string = recieve(this->server_fd);
        send_ACK(this->server_fd);

        size_t end_of_total_players_string = id_string.find('#');
        std::string total_players_string = id_string.substr(0,end_of_total_players_string);
        std::string pi_packedstring = id_string.substr(end_of_total_players_string, id_string.size());

        PlayerInfo temp(pi_packedstring);
        this->pi = temp;
        this->total_players = str_to_integer(total_players_string);
    }

    void print_connection_statement() const{
        std::stringstream ss;
        ss << "Connected as player " << this->pi.p_id << " out of " << this->total_players << " total players";
        print_statement(ss.str());
    }

    void unpack_neighboor_string(){
        std::string neighboor_string = recieve(this->server_fd);
        send_ACK(this->server_fd);

        std::string neighboor_string_partial = neighboor_string.substr(1, neighboor_string.size());
        size_t second_split = neighboor_string_partial.find('#');

        std::string left_packed_string = neighboor_string.substr(0, second_split+1);
        std::string right_packed_string = neighboor_string_partial.substr(second_split, neighboor_string_partial.size());

        PlayerInfo left_nb(left_packed_string);
        PlayerInfo right_nb(right_packed_string);

        this->pi_arr.push_back(left_nb);
        this->pi_arr.push_back(right_nb);
    }

    void connect_to_right_nbr(){
        port_num_right = pi_arr[1].sock_name;

        host_name_right_nbr = pi_arr[1].host_name.c_str();
        set_up_player_addrinfo(&hints_right_nbr);
        call_getaddrinfo(&hints_right_nbr,
                         &servinfo_right_nbr,
                         host_name_right_nbr,
                         port_num_right.c_str());

        right_nbr_fd = build_socket_fd(servinfo_right_nbr,
                                       host_name_right_nbr,
                                       port_num_right);

        connect_to_socket(right_nbr_fd,
                          servinfo_right_nbr,
                          host_name_right_nbr,
                          port_num_right);
        wait_for_ACK(right_nbr_fd);
        this->fd_vect.push_back(this->right_nbr_fd);
        this->pi_arr[1].fd = this->right_nbr_fd;
    }

    void listen_for_connection(){
        std::pair<int, std::string> left_nbr_fd_pair = listen_on_socket_players(player_fd, host_name_left_nbr, port_num_left, 1, server_fd);
        this->left_nbr_fd = left_nbr_fd_pair.first;
        this->fd_vect.push_back(this->left_nbr_fd);
        this->pi_arr[0].fd = this->left_nbr_fd;
    }

    void establish_neighbor_connections(){
        if(this->total_players>1){
            int listen_count = 0;
            int connect_count = 0;
            bool can_exit_loop = false;
            do{
                std::string message = recieve(this->server_fd);
                // ACK sent inside of listen and connect
                if(message=="LISTEN"){
                    if(listen_count>0){
                        std::cerr << "LISTEN passed to player more than once" << std::endl;
                    }

                    this->listen_for_connection();
                    send_ACK(this->server_fd);
                    listen_count++;
                    if(this->total_players==2){
                        this->listener_only = true;
                        this->connector_only = false;
                        break;
                    }
                }else if(message=="CONNECT"){
                    if(connect_count>0){
                        std::cerr << "CONNECT passed to player more than once" << std::endl;
                    }
                    this->connect_to_right_nbr();
                    send_ACK(this->server_fd);
                    connect_count++;
                    if(this->total_players==2){
                        this->listener_only = false;
                        this->connector_only = true;
                        break;
                    }
                }else{
                    std::cerr << "   Player received unexpected message while establishing connection with neighbors: " << message << std::endl;
                    exit(1);
                }

                if(total_players>2){
                    if(listen_count+connect_count==2){
                        can_exit_loop=true;
                    }
                }else{
                    can_exit_loop = true;
                }
            }while(!can_exit_loop);
        }
    }

    void send_setup_message_to_server() const{
        // need to wait for an ACK
        wait_for_ACK(this->server_fd);
        std::stringstream ss;
        ss << "Player " << this->pi.p_id << " is read to play";
        std::string msg = ss.str();
        send_message(this->server_fd, msg);
    }

    int get_fd_for_3_or_more_players(const std::string & who)const{
        int fd = -1;
        if(who == "server"){
            fd = this->server_fd;
        }else if (who == "left"){
            fd = this->left_nbr_fd;
        }else if (who == "right"){
            fd = this->right_nbr_fd;
        }else{
            //error
            std::cerr << " The specified recipient" << who << " does not exist" << std::endl;
            exit(1);
        }
        return fd;
    }

    int get_fd_for_2_players(const std::string & who)const{
        int fd = -1;
        if(who == "server"){
            fd = this->server_fd;
        }else if ((who == "left") || (who == "right")){
            if(this->listener_only){
                fd = this->left_nbr_fd;
            }else{
                fd = this->right_nbr_fd;
            }
        }else{
            //error
            std::cerr << " The specified recipient" << who << " does not exist" << std::endl;
        }
        return fd;
    }

    void send_message_by_recipeient_name(const std::string & msg, const std::string& who) const{

        int fd = -1;
        if(this->total_players==2){
            fd = this->get_fd_for_2_players(who);
        }else{
            fd = this->get_fd_for_3_or_more_players(who);
        }

        send_message(fd, msg);
        wait_for_ACK(fd);
        send_ACK(fd);
    }

    void print_end_of_game_statement() const{
        std::cout << "I'm it" << std::endl;
    }

    void send_end_game_message_and_potato_to_server(){
        std::stringstream ss;
        ss << "@" << pack_potato(hot_potato);
        send_message(this->server_fd, ss.str());

    }

    void update_potato(){
        bool game_is_not_over = at_least_one_hop_remaining(hot_potato);
        if(game_is_not_over){
            this->hot_potato = decrement_remaining_hops(this->hot_potato);
            this->hot_potato = append_new_owner(this->hot_potato, this->pi.p_id);

        }else{
            this->print_end_of_game_statement();
        }
    }

    void accept_and_update_potato(const std::string & potato_string){
        Potato temp = unpack_potato_string(potato_string);
        this->hot_potato = temp;

        this->update_potato();
    }

    bool does_message_say_to_continue_game(const std::string & flag_string){
        bool continue_game;
        if((flag_string == "$") | (flag_string == "*")){
            continue_game = true;
        }else if(flag_string == "@"){
            continue_game = false;
        }else{
            std::cerr << "Message received without an appropriate decision flag at the beginning" << std::endl;
            exit(1);
        }
        return continue_game;
    }

    void print_sending_potato_message(std::string who){
        std::stringstream ss;
        ss << "Sending potato to ";
        if(who=="left"){
            ss << pi_arr[0].p_id;
            print_statement(ss.str());
        }else if(who=="right"){
            ss << pi_arr[1].p_id;
            print_statement(ss.str());
        }
    }

    bool player_recieve_message(){
        std::vector<std::pair<int, std::string> > message_vector;
        message_vector = wait_for_any_messages(this->fd_vect);
        bool should_game_continue = true;
        for(size_t i = 0; i < message_vector.size(); i++){
            std::string message = message_vector[i].second;
            int fd_to_use = this->fd_vect[message_vector[i].first];

            send_ACK(fd_to_use);
            wait_for_ACK(fd_to_use);

            std::string flag_string = message.substr(0,1);
            should_game_continue = does_message_say_to_continue_game(flag_string);
            if(should_game_continue){
                // receive potato from other player
                accept_and_update_potato(message);

                if(this->total_players==1){
                    do{
                        this->update_potato();
                    }while(this->hot_potato.hops_remaining!=0);
                    should_game_continue = false;
                }

                // Decide if to end game here
                if(this->hot_potato.hops_remaining==0){
                    //end game
                    this->print_end_of_game_statement();
                    //should_game_continue = false;
                    this->send_end_game_message_and_potato_to_server();
                }else{
                    //send potato to other player
                    std::stringstream ss;
                    ss << pack_potato(hot_potato);
                    std::string msg = ss.str();

                    // get random neighbor
                    std::string who = random_get_left_or_right();
                    this->print_sending_potato_message(who);
                    this->send_message_by_recipeient_name(msg, who);
                }
            }else{
                return should_game_continue;
            }
        }
        return should_game_continue;
    }

    void close_client_connections(){
        std::string message = recieve(this->server_fd);

        if((this->total_players>2) || ((this->total_players==2) & connector_only)){
            if(message == "CLOSE_RIGHT"){
                close(this->right_nbr_fd);
            }
        }
        right_closed = true;
        send_ACK(this->server_fd);
    }

    void close_server_connections(){
        std::string message = recieve(this->server_fd);

        if((this->total_players>2) || ((this->total_players==2) & listener_only)){
            if(message == "CLOSE_LEFT_SERVER"){
                close(this->left_nbr_fd);
            }
        }
        left_closed = true;
        close(this->server_fd);
        server_closed = true;
    }

    void free_and_close(){
        freeaddrinfo(servinfo_server);
        if(!listener_only){
            freeaddrinfo(servinfo_left_nbr);
            freeaddrinfo(servinfo_right_nbr);// SIGSEGV for player 1 (listener)
        }
        if(!server_closed){
            close(server_fd);
        }
        if(!left_closed){
            close(left_nbr_fd);
        }
        if(!right_closed){
            close(right_nbr_fd);
        }
        close(player_fd);
    }

public:
    Player(const std::string & machine_nm,
           const std::string & port_number) : machine_name(machine_nm), port_num_server(port_number), pi_arr(0), left_closed(false), right_closed(false), server_closed(false){
        this->set_up_send_socket();
        this->build_listen_socket();
        this->send_player_listen_port_to_server();
        this->unpack_id_string();
        srand((unsigned int)time(NULL)+this->pi.p_id); // Seed random number generator. This is equalt to: srand((unsigned int)time(NULL)+player_id);
        this->print_connection_statement();
        this->unpack_neighboor_string();
        this->establish_neighbor_connections();
        this->send_setup_message_to_server();
    }

    ~Player(){
        free_and_close();
    }

    void play_game(){
        bool should_game_continue = true;
        while(should_game_continue){
            should_game_continue = this->player_recieve_message();
        }
        if(total_players>1){
            send_ACK(this->server_fd);
            this->close_client_connections();
            this->close_server_connections();
        }else{
            left_closed=true;
            right_closed=true;
            listener_only=true;
        }
    }
}; // END PLAYER CLASS

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Client
std::vector<std::string> initial_arg_checks(int argc, char *argv[]){
    int num_args = argc - 1; // remember that the first argv is the name of the main program, so if you have the user
    // specifics 3 command line arguments, you actually get 4 command line arguments passed into main
    if(num_args < 2 || num_args > 2){
        std::stringstream err_msg;
        err_msg << "Incorrect number of command line arguments passed to Player. Expected 2, got: "
                << argc << std::endl;
        throw std::invalid_argument(err_msg.str());
    }
    std::vector<std::string> initial_args;
    try{
        for(int i = 1; i < (num_args+1); i++){
            std::string new_arg = std::string(argv[i]);
            initial_args.push_back(new_arg);
        }
    }catch(std::invalid_argument & e){
        // check that the error catch and correction is correct
        std::stringstream err_msg;
        err_msg << "Invalid command line argument passed to Player. Expected a machine name and port number: "
                   "<machine_name> <port_num_server>, got something different." << std::endl;
        std::cout << "An exception (" << e.what() << ") occurred!\n" << err_msg.str();
        throw e;
    }

    if( (str_to_integer(initial_args[1]) <= 0) || (str_to_integer(initial_args[1]) > 65535)){
        std::stringstream err_msg;
        err_msg << "Invalid port number request. Port number must be >= 0 and <= 65535: "
                << initial_args[1] << std::endl;
        throw std::invalid_argument(err_msg.str());
    }
    return initial_args;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]){
    std::vector<std::string> initial_args = initial_arg_checks(argc, argv);
    std::string machine_name = initial_args[0];
    std::string port_num = initial_args[1];

    Player me(machine_name, port_num);
    me.play_game();
    return 0;
    // Add code to catch command line interupt, and exit gracefully
}

