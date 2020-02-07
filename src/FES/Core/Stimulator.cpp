#include <FES/Core/Stimulator.hpp>
#include <FES/Core/Channel.hpp>
#include <FES/Core/Event.hpp>
#include <Utility/Utility.hpp>
#include <MEL/Core/Console.hpp>
#include <MEL/Logging/Log.hpp>
#include <MEL/Core/Timer.hpp>
#include <MEL/Utility/System.hpp>
#include <Windows.h>
#include <tchar.h>

using namespace mel;

namespace fes{
    Stimulator::Stimulator(const std::string& name_, const std::string& com_port_, std::vector<Channel>& channels_):
        name(name_),
        com_port(com_port_),
        open(false),
        channels(channels_),
        scheduler()
    {
        enable();
    }

    Stimulator::~Stimulator(){
        if(is_enabled()){
            CloseHandle(hComm);
            LOG(Info) << "Stimulator Closed";
        }
    }

    // Open serial port ttyUSB0 and store parameters in fd0
    bool Stimulator::enable() {
        open = open_port(hComm); // open the comport with read/write permissions
        configure_port(hComm);    // Configure the parameters for serial port ttyUSB0
        initialize_board(hComm);  // Write stim board setup commands to serial port ttyUSB0
    }

    bool Stimulator::open_port(HANDLE& hComm_){

        std::string com_port_formatted = "\\\\.\\" + com_port;

        // the comport must be formatted as an LPCWSTR, so we need to get it into that form from a std::string
        std::wstring stemp = std::wstring(com_port_formatted.begin(), com_port_formatted.end());
        LPCWSTR com_port_lpcwstr = stemp.c_str();

        hComm_ = CreateFile(com_port_lpcwstr,            // port name
                           GENERIC_READ | GENERIC_WRITE, // Read/Write
                           0,                            // No Sharing
                           NULL,                         // No Security
                           OPEN_EXISTING,                // Open existing port only
                           0,                            // Non Overlapped I/O
                           NULL);                        // Null for Comm Devices

        // Check if creating the comport was successful or not and log it
        if (hComm_ == INVALID_HANDLE_VALUE){
            LOG(Error) << "Failed to open Stimulator" << get_name();
            return false;
        }
        else{
            LOG(Info) << "Opened Stimulator" << get_name();
        }

        return true;
    }

    bool Stimulator::configure_port(HANDLE& hComm_){  // configure_port establishes the settings for each serial port

        // http://bd.eduweb.hhs.nl/micprg/pdf/serial-win.pdf

        dcbSerialParams.DCBlength = sizeof(DCB);

        if (!GetCommState(hComm_, &dcbSerialParams)) {
            LOG(Error) << "Error getting serial port state";
            return false;
        }

        // set parameters to use for serial communication

        // set the baud rate that we will communicate at to 9600
        dcbSerialParams.BaudRate = CBR_9600;

        // 8 bits in the bytes transmitted and received.
        dcbSerialParams.ByteSize = 8;
        
        // Specify that we are using one stop bit
        dcbSerialParams.StopBits = ONESTOPBIT;

        // Specify that we are using no parity 
        dcbSerialParams.Parity = NOPARITY;

        // Disable all parameters dealing with flow control
        dcbSerialParams.fOutX = FALSE;
        dcbSerialParams.fInX  = FALSE;
        dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
        dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
        // dcbSerialParams.fOutxCtsFlow = FALSE;
        // dcbSerialParams.fOutxDsrFlow = FALSE;

        // Set communication parameters for the serial port
        if(!SetCommState(hComm_, &dcbSerialParams)){
            LOG(Error) << "Error setting serial port state";
            return false;
        }

        COMMTIMEOUTS timeouts={0};
        timeouts.ReadIntervalTimeout=50;
        timeouts.ReadTotalTimeoutConstant=50;
        timeouts.ReadTotalTimeoutMultiplier=10;
        timeouts.WriteTotalTimeoutConstant=50;
        timeouts.WriteTotalTimeoutMultiplier=10;
        if(!SetCommTimeouts(hComm_, &timeouts)){
            LOG(Error) << "Error setting serial port timeouts";
            return false;
        }

        return true;
    }

    bool Stimulator::initialize_board(HANDLE& hComm_){

        // delay time after sending setup messages of serial comm
        
        //Create byte array
        //Structure of byte arrays:
        //  Destination address - always 0x04
        //  Source address - always 0x80
        //  Message type - see message commands in header file
        //  Message Length - length of message without header (everything up to msg length) or checksum
        //  Message - Bytes of the message - must be of length Message Length
        //  Checksum Calculation - add each of the bytes, mask the lower byte of sum and add cary byte, then invert sum

        //Structure of channel setup messages:
        //  1 byte:  Port channel - lower 4 bits are channel, upper for bits are port (always 0): aka always 0x0a where a is channel number
        //  1 byte:  Amplitude limit - max amplitude the channel will outport
        //  1 byte:  Pulse Width Limit - max pulse with that channel will output
        //  2 bytes: Interphase Delay in usec - space between phases of the waveform (can be from 10-65535)
        //  1 byte:  Aspect Ratio - designates the proportion of the amplitude of the first phase to the second phase.
        //             - The lower 4 bits represent the first phase and the upper 4 bits represent the second phase. 0x11 is 1 to 1 ratio
        //  1 byte:  AnodeCathode - for 4 bipolar channels (what we have) these are 0x01, 0x23, 0x45, 0x67 respectively
        
        for (auto i = 0; i < channels.size(); i++){
            if (!channels[i].setup_channel(hComm_, delay_time)){
                return false;
            };
        }

        LOG(Info) << "Setup Completed successfully.";

        return true;
    }

    bool Stimulator::write_setup_message(HANDLE& handle_, unsigned char setup_message_[], std::string message_string_){

        DWORD dwBytesWritten = 0; // Captures how many bits were written

        // Generate checksum
        setup_message_[(sizeof(setup_message_) / sizeof(*setup_message_)) - 1] = checksum(setup_message_, (sizeof(setup_message_) / sizeof(*setup_message_)));

        // Attempt to send setup message and log whether it was successful or not
        if(!WriteFile(handle_, setup_message_, (sizeof(setup_message_) / sizeof(*setup_message_)),&dwBytesWritten,NULL)){
            LOG(Error) << "Error in " << message_string_ << "Setup.";
            return false;
        }
        else{
            LOG(Info) << message_string_ << "Setup was Successful.";
        }

        // Sleep for delay time to allow the board to process
        sleep(delay_time);
        return true;
    }

    bool Stimulator::create_scheduler(const unsigned char sync_msg , unsigned int duration){
        return scheduler.create_scheduler(hComm, sync_msg, duration, delay_time);
    }

    bool Stimulator::is_enabled(){
        return open;
    }

    std::string Stimulator::get_name(){
        return name;
    }
}