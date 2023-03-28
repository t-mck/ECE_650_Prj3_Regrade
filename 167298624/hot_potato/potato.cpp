#include "potato.hpp"

int str_to_integer(std::string number_string){
    std::stringstream ss;
    ss << number_string;
    int rtr = 0;
    ss >> rtr;
    return rtr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////          Potato            ///////////////////////////////////////////////////

std::string get_trace_of_potato(Potato hot_potato){
    std::stringstream ss;
    ss << "Trace of potato:\n";
    ss << hot_potato.trace << std::endl;
    return ss.str();
}

Potato append_new_owner(Potato hot_potato, int p_id){
    std::stringstream ss;
    std::string current_trace(hot_potato.trace);
    if(!current_trace.empty()){
        ss << hot_potato.trace << "," << p_id;
    }else{
        ss << p_id;
    }

    hot_potato.trace = ss.str();
    return hot_potato;
}

bool at_least_one_hop_remaining(Potato hot_potato){
    if(hot_potato.hops_remaining > 0){
        return true;
    }
    return false;
}

Potato decrement_remaining_hops(Potato hot_potato){
    assert(hot_potato.hops_remaining > 0);
    hot_potato.hops_remaining = hot_potato.hops_remaining - 1;
    return hot_potato;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////          Pack/Unpack            ////////////////////////////////////////////////

std::string pack_potato(Potato hot_potato){
    std::stringstream potato_string;
    potato_string << "$" << hot_potato.hops_remaining << "|" << hot_potato.trace;
    return potato_string.str();
}

Potato unpack_potato_string(std::string potato_string_with_flag){
    std::string potato_flag = potato_string_with_flag.substr(0,1);
    if(potato_flag == "$"){
        std::string potato_string = potato_string_with_flag.substr(1,potato_string_with_flag.size());
        int end_of_hop_string = potato_string.find('|');
        std::string hop_string = potato_string.substr(0,end_of_hop_string);
        std::string trace_string = potato_string.substr(end_of_hop_string+1, potato_string.size());
        Potato temp;
        temp.hops_remaining = str_to_integer(hop_string);
        temp.trace = trace_string;
        return temp;
    }
    std::cerr << "potato flag not found during unpacking of potato string" << std::endl;
    exit(1);
}