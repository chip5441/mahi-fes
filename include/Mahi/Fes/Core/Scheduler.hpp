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

#pragma once

#include <Mahi/Fes/Core/Channel.hpp>
#include <Mahi/Fes/Core/Event.hpp>
#include <Mahi/Util.hpp>
#include <vector>

#define DEL_SCHED_LEN 0x01
#define STIM_EVENT    0x03

namespace mahi {
namespace fes {

class Scheduler {
public:
    /// Scheduler constructor
    Scheduler();
    /// Scheduler destructor
    ~Scheduler();
    /// creates the scheduler object
    bool create_scheduler(HANDLE& hComm_, const unsigned char sync_msg, unsigned int duration,
                          mahi::util::Time setup_time);
    /// add an event to the stimulator, and sleep for a short time to let the UECU process
    bool add_event(Channel channel_, mahi::util::Time sleep_time, bool is_virtual_,
                   unsigned char event_type = STIM_EVENT);
    /// enable the scheduler
    void enable();
    /// disable the scheduler
    void disable();
    /// return the scheduler ID
    unsigned char get_id();
    /// set the scheduler ID
    void set_id(unsigned char sched_id_);
    /// write a new amplitude to a specified channel
    void set_amp(Channel channel_, unsigned int amplitude_);
    /// return the amplitude of a specified channel
    unsigned int get_amp(Channel channel_);
    /// set a new pulsewidth value for a specified channel
    void write_pw(Channel channel_, unsigned int pw_);
    /// return the pulsewidth value of a specified channel
    unsigned int get_pw(Channel channel_);
    /// return the number of events attached to the scheduler
    size_t get_num_events();
    /// return the vector of events for the scheduler
    std::vector<Event> get_events();
    /// send the message to halt the scheduler -> stopping all events attached to it
    bool halt_scheduler();
    /// command each of the events to write it's current pw and amplitude to the
    bool update();
    /// send the sync message to start commanding the events attached to it
    bool send_sync_msg();
    /// return whether or not the scheduler is enabled
    bool is_enabled();

private:
    unsigned char      m_id;         // the schedule id
    std::vector<Event> m_events;     // vector of events for the current scheduler
    bool               m_enabled;    // value indicating whether the scheduler is currently enabled
    HANDLE             m_hComm;      // serial handle to the appropriate UECU
    unsigned char      m_sync_char;  // sync message for the scheduler which tells it to begin
};
}  // namespace fes
}  // namespace mahi