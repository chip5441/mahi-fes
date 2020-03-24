// MIT License
//
// Copyright (c) 2020 Mechatronics and Haptic Interfaces Lab - Rice University
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// Author(s): Nathan Dunkelberger (nbd2@rice.edu)

#include <Mahi/Util.hpp>
#include <vector>

#include "Windows.h"

using namespace mahi::util;

namespace mahi {
namespace fes {
    
// checksum is a function that preforms checksums of all of the unsigned char arrays used in this
// code
int checksum(unsigned char myarray[], int array_size) {
    int csum = 0;
    for (int i = 0; i < array_size - 1; i++) {
        csum += myarray[i];
    }
    csum = ((0x00FF & csum) + (csum >> 8)) ^ 0xFF;
    return csum;
}

std::string print_as_hex(unsigned char num){
    char char_buff[20];
    sprintf(char_buff, "0x%02X", (unsigned int)num);
    return std::string(char_buff);
}

std::vector<unsigned char> int_to_twobytes(int input_int) {
    // value to do comparison
    int byte_size = 256;

    // initialize empty vector of unsigned char
    std::vector<unsigned char> char_vec_out;

    // integer division for the first byte (eg. 0 if < 256, 1 if >=256 && < 512, 2 if >=512 && <
    // 768, etc)
    char_vec_out.push_back(input_int / byte_size);

    // remainder after dividing by 256 (will always be between 1 and 256)
    char_vec_out.push_back(input_int % byte_size);

    return char_vec_out;
}

bool write_message(HANDLE hComm_, unsigned char* message_, const int message_size_,
                   const std::string& activity) {
    // dont log anything if the input string is "NONE"
    bool log_message = (activity.compare("NONE") != 0);

    // Captures how many bits were written
    DWORD dwBytesWritten = 0;

    // add the checksum to the last message
    message_[message_size_ - 1] = (unsigned char)checksum(message_, message_size_);

    // write the file iff possible
    if (!WriteFile(hComm_, message_, message_size_, &dwBytesWritten, NULL)) {
        if (log_message) {
            LOG(Error) << "Error " << activity;
        }
        return false;
    } else {
        // for (auto i = 0; i < message_size_; i++){
        //     char char_buff[4];
        //     sprintf(char_buff, "0x%02X", (unsigned int)message_[i]);
        //     std::cout << char_buff;
        //     if(i != (message_size_-1)) std::cout << ", ";
        //     else std::cout << std::endl;
        // }

        if (log_message) {
            LOG(Info) << activity << " was Successful.";
        }
        return true;
    }
}
}  // namespace fes
}  // namespace mahi