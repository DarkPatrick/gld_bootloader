#pragma once


#include <conio.h>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include "crc.h"
#include "communication.h"


enum PackSize {
    XMODEM_SIZE = 128, XMODEM_1K_SIZE = 1024, UNLIMITED_SIZE = UINT32_MAX
};


enum HeaderBytes {
    SOH = 0x01, STX = 0x02, EOT = 0x04, ETB = 0x017, CAN = 0x018
};


enum ServiceCmds {
    ACK = 0x06, NAK = 0x15, C = 0x43, CTRL_Z = 0x1A
};


enum Protocol {
    PR_NONE, PR_XMODEM, PR_XMODEM_1K
};


class XModem {
    public:
        XModem();
        XModem(CRC crc_type);
        XModem(const std::vector<uint8_t> data, const CRC crc_type = CRC::CRC_8);
        ~XModem();

        void setDefaultCrcType(const CRC crc_type);
        void openPort(const SPortParams port_params);
        void setDataBlock(std::vector<uint8_t> &data);
        uint32_t sendPack(std::vector<uint8_t> &data, const Protocol protocol = Protocol::PR_XMODEM_1K, const uint8_t clear_bufer = 0);
        void cancelTransmission();
        void restartTransmission();
        DCB getDCB() const;
        std::string getPortName() const;
        void setFileName(const std::string &file_name);
        std::string getFileName() const;
        uint32_t transmitFile(const Protocol protocol);
        uint32_t startTerminalMode() const;
    private:
        std::vector<uint8_t> wrapData(HeaderBytes first_byte);
        uint32_t loadFile(const Protocol protocol);
        void terminalOutput(uint8_t byte) const;
    public:
        Errors err;
    private:
        std::unique_ptr<CPort> port;
        std::vector<uint8_t> raw_data;
        std::vector<std::vector<uint8_t>> file_data;
        uint8_t pack_num;
        CRC default_crc;
        std::string file_name;
};
