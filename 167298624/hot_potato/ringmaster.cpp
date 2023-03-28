#include <vector>       //vector
#include <string>       //string
#include <cstring>      //memset
#include <iostream>     //std::cout
#include <netdb.h>      //addrinfo
#include <unistd.h>     //close
#include <map>
#include <cstdlib>

#include "socketutils.h"
// Server

class Ringmaster{
private:
    // Initial Args
    std::string port_num;
    std::string num_players;
    std::string num_hops;

    Potato hot_potato;

    // Listen socket
    struct addrinfo hints_listen;
    struct addrinfo *servinfo_listen; // point to the results
    char *host_name_listen;
    int listen_socket_file_descriptor;
    std::vector<int> listen_client_con_fds;

    // PlayerInfo Array
    std::vector<PlayerInfo> pi_arr;

    void print_ringmaster_gamesetup(){
        std::stringstream player_string;
        std::stringstream hop_string;

        player_string << "Players = " << num_players << std::endl;
        hop_string << "Hops = " << num_hops << std::endl;

        std::cout << "Potato Ringmaster" << std::endl;
        std::cout << player_string.str();
        std::cout << hop_string.str();
    }

    void set_up_socket_to_listen_on(){
        this->host_name_listen = NULL;
        set_up_ringmaster_addrinfo(&hints_listen);

        call_getaddrinfo(&hints_listen,
                         &servinfo_listen,
                         host_name_listen,
                         port_num);

        this->listen_socket_file_descriptor = build_socket_fd(servinfo_listen,
                                                              host_name_listen,
                                                              port_num);                                                // 3. Build the socket

        bind_socket(this->listen_socket_file_descriptor,
                    servinfo_listen,
                    host_name_listen,
                    port_num);                                                                                          // 4. bind the socket to the port
    }

    void listen_for_initial_player_connections(){
        std::vector<std::pair<int, std::string> > client_con_info;
        client_con_info = listen_on_socket_for_all_players(this->listen_socket_file_descriptor,
                                                           host_name_listen,
                                                           port_num,
                                                           str_to_integer(num_players));                              // 5. listen on sockets for player connections
        for(size_t i =0; i < client_con_info.size(); i++){
            this->listen_client_con_fds.push_back(client_con_info[i].first);
            std::stringstream ss;
            ss << "#" << i << "|" << client_con_info[i].second;
            PlayerInfo temp(ss.str());
            this->pi_arr.push_back(temp);
        }
    }

    void get_player_to_player_port_numbers_from_players(){
        for(size_t i =0; i < listen_client_con_fds.size(); i++){

            send_ACK(listen_client_con_fds[i]);
            std::string message = recieve(listen_client_con_fds[i]);

            send_ACK(listen_client_con_fds[i]);
            wait_for_ACK(listen_client_con_fds[i]);

            this->pi_arr[i].update_sock_name(message);
        }
    }

    std::vector<std::string> build_player_id_number_strings(){
        std::vector<std::string> message_vector;
        for(size_t i = 0; i < listen_client_con_fds.size(); i++){
            std::stringstream ss;
            ss << this->num_players << this->pi_arr[i].packed_string;
            std::string message = ss.str();
            message_vector.push_back(message);
        }
        return message_vector;
    }

    void build_players_neighboors_info(){
        size_t first_player = 0;
        size_t last_player = listen_client_con_fds.size() - 1;

        if(first_player==last_player){
            // only one player.. That player is it's own left and right neighbor
            this->pi_arr[0].update_nbrs(pi_arr[0].packed_string, pi_arr[0].packed_string);
        }else{
            for(size_t i = 0; i < listen_client_con_fds.size(); i++){
                std::string left_player_info;
                std::string right_player_info;
                if(i == first_player){
                    left_player_info = pi_arr[pi_arr.size()-1].packed_string;
                    right_player_info = pi_arr[i+1].packed_string;
                }else if(i == last_player){
                    left_player_info = pi_arr[i-1].packed_string;
                    right_player_info = pi_arr[0].packed_string;
                }else{
                    left_player_info = pi_arr[i-1].packed_string;
                    right_player_info = pi_arr[i+1].packed_string;
                }
                this->pi_arr[i].update_nbrs(left_player_info, right_player_info);
            }
        }
    }

    std::vector<std::string> build_nbr_message_vector(){
        std::vector<std::string> message_vector;
        for(size_t i = 0; i < listen_client_con_fds.size(); i++) {
            std::string neighboor_string = this->pi_arr[i].get_nbr_message_string();
            message_vector.push_back(neighboor_string);
        }
        return message_vector;
    }


    void set_up_all_players(){
        // 1. Send Player's their ID information

        std::vector<std::string> id_message_vector =  this->build_player_id_number_strings();

        select_send_messages_to_multiple_fds(id_message_vector, this->listen_client_con_fds);
        wait_for_multiple_ACK(this->listen_client_con_fds);

        // 2. Send Player's their neighbor information
        this->build_players_neighboors_info();
        std::vector<std::string> nbr_message_vector = this->build_nbr_message_vector();

        select_send_messages_to_multiple_fds(nbr_message_vector, this->listen_client_con_fds);
        wait_for_multiple_ACK(this->listen_client_con_fds);
    }

    void establish_connection_between_players(){
        size_t total_players = str_to_integer(this->num_players);
        std::string LISTEN = "LISTEN";
        std::string CONNECT = "CONNECT";
        if(total_players == 2 ){

            send_message(listen_client_con_fds[1], LISTEN);
            wait_for_ACK(listen_client_con_fds[1]);

            send_message(listen_client_con_fds[0], CONNECT);
            wait_for_ACK(listen_client_con_fds[0]);

            wait_for_ACK(listen_client_con_fds[1]);

        }else if (total_players > 2){ // 3 or more players
            for(size_t i = 1; i < listen_client_con_fds.size(); i++){

                send_message(listen_client_con_fds[i], LISTEN);
                wait_for_ACK(listen_client_con_fds[i]);

            }
            for(size_t i = 0; i < listen_client_con_fds.size(); i++){
                if(i==0){

                    send_message(listen_client_con_fds[i], CONNECT);
                    wait_for_ACK(listen_client_con_fds[i]);

                    send_message(listen_client_con_fds[i], LISTEN);
                    wait_for_ACK(listen_client_con_fds[i]);

                }else{
                    wait_for_ACK(listen_client_con_fds[i]); //

                    send_message(listen_client_con_fds[i], CONNECT);
                    wait_for_ACK(listen_client_con_fds[i]);

                }
            }
            wait_for_ACK(listen_client_con_fds[0]); //wait for final ACK from player 0
        }
        // No neighbor connections necessary for 0 or 1 players
    }

    void wait_for_player_ready_message(){
        for(size_t i = 0; i < listen_client_con_fds.size(); i ++){

            send_ACK(listen_client_con_fds[i]);
            std::string ready_msg = recieve(listen_client_con_fds[i]);
            print_statement(ready_msg);
        }
    }

    void free_and_close(){
        freeaddrinfo(servinfo_listen);
        close(listen_socket_file_descriptor); /* Close the file descriptor FD.*/
        for(size_t i = 0; i < listen_client_con_fds.size(); i++){
            close(listen_client_con_fds[i]);
        }
    }

    void print_potato_trace(const Potato & tater){
        std::cout << get_trace_of_potato(tater);
    }

public:
    Ringmaster(const std::string & port_number,
               const std::string & number_players,
               const std::string & number_hops) : port_num(port_number), num_players(number_players), num_hops(number_hops), pi_arr(0){

        Potato temp;
        temp.hops_remaining = str_to_integer(this->num_hops);
        temp.trace = "";
        this->hot_potato = temp;
        srand((unsigned int)time(NULL)+ str_to_integer(number_players));
        this->print_ringmaster_gamesetup();
        this->set_up_socket_to_listen_on();
        this->listen_for_initial_player_connections();
        this->get_player_to_player_port_numbers_from_players();
        this->set_up_all_players();
        this->establish_connection_between_players();
        this->wait_for_player_ready_message();
    }

    ~Ringmaster(){
        this->free_and_close();
    }

    void start_game(){
        int starting_player = get_random_player(num_players, str_to_integer(num_players)+1);                     // 6. randomly select first player
        std::stringstream ss;
        ss << "Ready to start the game, sending potato to player " << starting_player;
        print_statement(ss.str());

        send_potato(listen_client_con_fds[starting_player], this->hot_potato);                                                               // 7. send potato to first player
    }

    static std::pair<bool, std::string> recieve_end_of_game_messages(std::vector<std::pair<int, std::string> > message_vector){
        std::pair<bool, std::string> rtr_pair;
        for(size_t i = 0; i < message_vector.size(); i++){
            std::string message = message_vector[i].second;
            std::string message_flag = message.substr(0,1);
            if(message_flag == "@"){
                rtr_pair.first=true;
                rtr_pair.second = message.substr(1,message.size());
                return rtr_pair;
            }
        }
        rtr_pair.first = false;
        rtr_pair.second = "";
        return rtr_pair;
    }

    void send_end_of_game_message(){
        std::string END = "@";
        for(size_t i = 0; i < this->listen_client_con_fds.size(); i++){

            send_message(listen_client_con_fds[i], END);

            wait_for_ACK(listen_client_con_fds[i]);

            send_ACK(listen_client_con_fds[i]);

        }

    }

    void tell_players_to_close_connections(){
        int total_players = str_to_integer(this->num_players);
        if(total_players>1){
            wait_for_multiple_ACK(listen_client_con_fds);
            std::string CLOSE_RIGHT = "CLOSE_RIGHT";
            std::string CLOSE_LEFT_SERVER = "CLOSE_LEFT_SERVER";
            for(size_t i = 0; i < listen_client_con_fds.size(); i++){
                send_message(listen_client_con_fds[i], CLOSE_RIGHT);

                wait_for_ACK(listen_client_con_fds[i]);

            }
            for(size_t i = 0; i < listen_client_con_fds.size(); i++){

                send_message(listen_client_con_fds[i], CLOSE_LEFT_SERVER);

            }
        }
    }

    void end_game(){
        int number_of_hops = str_to_integer(this->num_hops);
        int total_players = str_to_integer(this->num_players);
        if(number_of_hops > 0){
            bool end_of_game_flag_found= false;
            do{
                std::vector<std::pair<int, std::string> > message_vector;
                do{
                    message_vector = wait_for_any_messages(listen_client_con_fds);
                }while(message_vector.empty()); // This is a computationally expensive way to do this, try setting the time frame to infinity instead.

                std::pair<bool, std::string> end_of_game_pair = recieve_end_of_game_messages(message_vector);
                if(end_of_game_pair.first){
                    //send end of game message to players

                    if(total_players>1){
                        send_end_of_game_message();
                        tell_players_to_close_connections();
                    }

                    // update potato
                    Potato temp = unpack_potato_string(end_of_game_pair.second);
                    this->hot_potato = temp;

                    // print potato trace
                    print_potato_trace(this->hot_potato);
                    end_of_game_flag_found = true;
                }else{
                    //dropped player
                }
            }while(!end_of_game_flag_found);
        }else{
            send_end_of_game_message();
            if(total_players>1){
                tell_players_to_close_connections();
            }
        }
    }

}; // END RINGMASTER CLASS

/**
 * Checks that the requested port number is an acceptable value
 *
 * @param initial_args std::vector<int> the command line arguments supplied to the program
 * @throws std::invalid_argument if the port number is invalid, an invalid number of players is provided, or an invalid number of hops is specified
 */
void intial_args_value_checks(std::vector<std::string> initial_args){
    if( (str_to_integer(initial_args[0]) <= 0) || (str_to_integer(initial_args[0]) > 65535)){
        std::stringstream err_msg;
        err_msg << "Invalid port number request. Port number must be >= 0 and <= 65535: "
                << initial_args[0] << std::endl;
        throw std::invalid_argument(err_msg.str());
    }

    if(str_to_integer(initial_args[1]) < 1){
        std::stringstream err_msg;
        err_msg << "Invalid number of players supplied to Ringmaster. Must be 1 or more got: "
                << initial_args[1] << std::endl;
        throw std::invalid_argument(err_msg.str());
    }

    if( (str_to_integer(initial_args[2]) > 512) | (str_to_integer(initial_args[2]) < 0)){
        std::stringstream err_msg;
        err_msg << "Invalid number of hops supplied to Ringmaster. Must be >= 0 and < 512, got: "
                << initial_args[2] << std::endl;
        throw std::invalid_argument(err_msg.str());
    }
}

/**
 * This function checks that an appropriate number of command line arguments were supplied, and that they are of the
 * correct type
 * @param argc
 * @param argv
 * @throws std::invalid_argument if an incorrect number of arguments are supplied, or the arguments are of the incorrect type
 * @return
 */
std::vector<std::string> check_argv_size_get_arg_vector(int argc, char *argv[]){
    int num_args = argc - 1; // remember that the first argv is the name of the main program, so if you have the user
    // specifics 3 command line arguments, you actually get 4 command line arguments passed
    // into main
    if(num_args < 3 || num_args > 3){
        std::stringstream err_msg;
        err_msg << "Incorrect number of command line arguments passed to Ringmaster. Expected 3, got: "
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
        err_msg << "Invalid command line argument passed to Ringmaster. Expected three numbers: "
                   "<port_num_server> <num_players> <num_hops>, got something different." << std::endl;
        std::cout << "An exception (" << e.what() << ") occurred!\n" << err_msg.str();
        throw e;
    }
    return initial_args;
}

/**
 *
 * @param argc
 * @param argv
 * @throws std::invalid_argument
 * @return
 */
std::vector<std::string> initial_arg_checks_and_conversion(int argc, char *argv[]){
    std::vector<std::string> initial_args = check_argv_size_get_arg_vector(argc, argv);
    intial_args_value_checks(initial_args);
    return initial_args;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]){
    std::vector<std::string> initial_args = initial_arg_checks_and_conversion(argc, argv);
    std::string port_num = initial_args[0];
    std::string num_players = initial_args[1];
    std::string num_hops = initial_args[2];

    Ringmaster ring(port_num, num_players, num_hops);
    if(str_to_integer(num_hops) > 0){
        ring.start_game();
    }
    ring.end_game();

    return 0;
}
