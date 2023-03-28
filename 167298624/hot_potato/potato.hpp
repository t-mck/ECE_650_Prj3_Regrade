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

struct potato{
    int hops_remaining;
    std::string trace;
}; typedef struct potato Potato;

int str_to_integer(std::string number_string);

std::string get_trace_of_potato(Potato hot_potato);

Potato append_new_owner(Potato hot_potato, int p_id);

bool at_least_one_hop_remaining(Potato hot_potato);

Potato decrement_remaining_hops(Potato hot_potato);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////          Pack/Unpack            ////////////////////////////////////////////////

std::string pack_potato(Potato hot_potato);

Potato unpack_potato_string(std::string potato_string_with_flag);