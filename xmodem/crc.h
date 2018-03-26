#pragma once


#include <vector>


enum CRC {
    CRC_8, CRC_16
};


#define ONE_BYTE_SHIFT 8
#define BYTE_MASK      0xFF


std::vector<uint8_t> evalCRC(const std::vector<uint8_t> data, CRC crc_type);
