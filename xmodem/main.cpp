#include "main.h"


uint8_t fromStringToHex(const std::string& s) {
    std::istringstream iss(s);
    uint32_t res;

    iss >> std::hex >> res;

    return res & 0xFF;
}


SPortParams readPortParams(const std::vector<std::string> &params, std::string &file_name) {
    SPortParams port_params;

    file_name = "";
    port_params.port_name = "COM0";
    port_params.dcb.BaudRate = 115200;
    port_params.dcb.ByteSize = 8;
    port_params.dcb.Parity = NOPARITY;
    port_params.dcb.StopBits = ONESTOPBIT;

    for (auto i : params) {
        std::transform(i.begin(), i.end(), i.begin(), ::tolower);
        if (i.find("com") != std::string::npos) {
            port_params.port_name = i;
        } else if ((i.find(".") != std::string::npos) ||
                (i.find("\"") != std::string::npos) ||
                (i.find("'") != std::string::npos)) {
            i.erase(std::remove(i.begin(), i.end(), '\"'), i.end());
            i.erase(std::remove(i.begin(), i.end(), '\''), i.end());
            file_name = i;
        } else if (std::string::npos != i.find("no")) {
            port_params.dcb.Parity = NOPARITY;
        } else if (std::string::npos != i.find("odd")) {
            port_params.dcb.Parity = ODDPARITY;
        } else if (std::string::npos != i.find("even")) {
            port_params.dcb.Parity = EVENPARITY;
        } else if (std::string::npos != i.find("mark")) {
            port_params.dcb.Parity = MARKPARITY;
        } else if (std::string::npos != i.find("space")) {
            port_params.dcb.Parity = SPACEPARITY;
        } else if (std::string::npos != i.find("one")) {
            port_params.dcb.StopBits = ONESTOPBIT;
        } else if (std::string::npos != i.find("half")) {
            port_params.dcb.StopBits = ONE5STOPBITS;
        } else if (std::string::npos != i.find("two")) {
            port_params.dcb.StopBits = TWOSTOPBITS;
        }

        uint32_t some_num = atoi(i.c_str());

        if ((some_num > 3) && (some_num < 9)) {
            port_params.dcb.ByteSize = some_num;
        }
        if (some_num > 8) {
            port_params.dcb.BaudRate = some_num;
        }
    }

    return port_params;
}


void printData(XModem &protocol) {
    std::vector<std::string> parity{
        "нет",
        "по нечётности",
        "по чётности",
        "по единичному биту",
        "по нулевому биту"
    },
        stop_bits{ "один", "полтора", "два" };

    DCB dcb = protocol.getDCB();
    std::vector<std::string> captions{
        "ООО «НПК «Электрооптика» ",
        "GLD_BOOTLOADER © ",
        "версия: 1.1.0 "
    };

    captions.push_back("порт: " + protocol.getPortName() +
        "; " + std::to_string(dcb.BaudRate) +
        " бод; " + std::to_string(dcb.ByteSize) +
        " байт; контроль: " +
        parity.at(dcb.Parity) +
        "; стоповые биты: " +
        stop_bits.at(dcb.StopBits)
    );
    captions.push_back("имя файла: " + protocol.getFileName());

    std::string caption = captions.at(0) +
        captions.at(1) +
        protocol.getPortName() +
        "; " +
        std::to_string(dcb.BaudRate);

    SetConsoleTitle(caption.c_str());

    for (auto i : captions) {
        std::cout << i << std::endl;
    }
}


uint32_t flashUpdate(const uint32_t gld_num, const std::vector<std::string> &params) {
    XModem protocol(CRC::CRC_16);
    std::string file_name;

    protocol.openPort(readPortParams(params, file_name));
    protocol.setFileName(file_name);

    printData(protocol);

    if (protocol.err.getErrorsNum()) {
        std::string err_msg;

        do {
            err_msg = protocol.err.getLastError();
            std::cout << err_msg << std::endl;
        } while (err_msg != "");

        return 0;
    }

    std::vector<uint8_t> gld_cmd;
    uint8_t terminal = 0;

    for (auto i : params) {
        if ((std::string::npos != i.find("0x")) || (std::string::npos != i.find("0X"))) {
            gld_cmd.push_back(fromStringToHex(i));
        } else {
            // TODO: разобраться с очисткой буфера
            if (gld_cmd.size()) {
                protocol.sendPack(gld_cmd, Protocol::PR_NONE, 1);
                //protocol.port->sendDataPack(gld_cmd, 1);
                std::vector<uint8_t>().swap(gld_cmd);
            }
            if (std::string::npos != i.find("terminal")) {
                terminal = 1;
            }
        }
    }
    if (gld_cmd.size()) {
        //protocol.port->sendDataPack(gld_cmd, 1);
        protocol.sendPack(gld_cmd, Protocol::PR_NONE, 1);
        std::vector<uint8_t>().swap(gld_cmd);
    }

    protocol.transmitFile(Protocol::PR_XMODEM_1K);

    if (terminal) {
        protocol.startTerminalMode();
    }

    //protocol.~XModem();

    return 1;
}


uint32_t main(const uint32_t argc, char** argv) {
    std::vector<std::string> cmd_params;

    for (uint32_t i = 1; i < argc; ++i) {
        cmd_params.push_back(static_cast<std::string>(argv[i]));
    }

    setlocale(LC_ALL, "rus");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    flashUpdate(0, cmd_params);

    std::cout << "нажми 'enter' для выхода..." << std::endl;
    getchar();

    return 0;
}
