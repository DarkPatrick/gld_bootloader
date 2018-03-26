#include "communication.h"


CPort::CPort(SPortParams port_params) {
    h_com = CreateFile(("\\\\.\\" + port_params.port_name).c_str(), GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL);

    if (h_com == INVALID_HANDLE_VALUE) {
        err.addError("error: cannot open com port...");
        return;
    }

    //*
    COMMTIMEOUTS com_timeouts;
    com_timeouts.ReadIntervalTimeout = 100;
    com_timeouts.ReadTotalTimeoutConstant = 100;
    com_timeouts.ReadTotalTimeoutMultiplier = 100;

    if (!SetCommTimeouts(h_com, &com_timeouts)) {
        err.addError("error: cannot setup com timeouts...");
        return;
    }
    //*/

    //GetCommState(h_com, &this->port_params.dcb);
    this->port_params.port_name = port_params.port_name;
    this->port_params.dcb = port_params.dcb;

    if (!SetCommState(h_com, &this->port_params.dcb)) {
        err.addError("error: fail to setup com state...");
        return;
    }

    //GetCommState(h_com, &dcb);
}


CPort::~CPort() {
    closeCommunication();
}


std::string CPort::clearInputBuffer() {
    std::string ret_val = "";
    uint8_t got_one = 1;

    while (got_one) {
        ret_val.push_back(readOneSymbol<uint8_t>(got_one));
    }

    return ret_val;
}


void CPort::closeCommunication() {
    CloseHandle(h_com);
    err.clearAllErrors();
}


SPortParams CPort::getParams() {
    return port_params;
}
