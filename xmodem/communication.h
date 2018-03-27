#pragma once


#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include "errors.h"


struct SPortParams {
    std::string port_name;
    DCB dcb;
};


class CPort {
    public:
        CPort(SPortParams port_params);
        ~CPort();
        template<typename T>
        T readOneSymbol(uint8_t &got_one) const;
        std::string clearInputBuffer() const;
        template<typename T>
        std::string sendDataPack(std::vector<T> data, const uint8_t clear_bufer = 0) const;
        void closeCommunication();
        SPortParams getParams() const;

        Errors err;
    private:
        HANDLE h_com;
        SPortParams port_params;
};


template<typename T>
T CPort::readOneSymbol(uint8_t &got_one) const {
    DWORD bytes_read;
    T buff;

    ReadFile(h_com, &buff, sizeof(buff), &bytes_read, nullptr);

    got_one = bytes_read ? 1 : 0;

    return buff;
}


template<typename T>
std::string CPort::sendDataPack(std::vector<T> data, const uint8_t clear_bufer) const {
    DWORD bytes_count;

    for (auto i : data) {
        WriteFile(h_com, &i, sizeof(T), &bytes_count, nullptr);
    }

    if (clear_bufer) {
        return clearInputBuffer();
    }
    
    return "";
}
