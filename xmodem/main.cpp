#include "main.h"


const std::string version = "2.0.0";


void launchUpdater(std::vector<std::string> &cmd_params) {
    TCHAR sz_path[] = "Software\\gld_bootloader\\";
    HKEY h_key;
    std::string sz_params;
    DWORD dw_buf_len;

    for (auto param : cmd_params) {
        sz_params.append(param + '\n');
    }

    RegCreateKeyEx(HKEY_CURRENT_USER, sz_path, 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE, NULL, &h_key, NULL);
    RegSetValueEx(h_key, "cmd params", 0, REG_SZ, (BYTE*)sz_params.c_str(), sz_params.size());
    RegCloseKey(h_key);
    system(("start updater.exe DarkPatrick gld_bootloader " + version + " gld_boot_x86.exe").c_str());
    RegOpenKeyEx(HKEY_CURRENT_USER, sz_path, 0, KEY_ALL_ACCESS, &h_key);
    RegQueryValueEx(h_key, "cmd_params", NULL, NULL, (BYTE*)sz_params.c_str(), &dw_buf_len);

    cmd_params.clear();

    std::string param = "";

    for (auto ch : sz_params) {
        if (ch != '\n') {
            param += ch;
        } else {
            cmd_params.push_back(param);
            param = "";
        }
    }
}



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
        "no",
        "odd",
        "even",
        "mark",
        "space"
    },
        stop_bits{ "one", "half as", "two" };

    DCB dcb = protocol.getDCB();
    std::vector<std::string> captions{
        "\"Electrooptica\" ",
        "GLD_BOOTLOADER ",
        "version: " + version + " "
    };

    captions.push_back("port: " + protocol.getPortName() +
        "; " + std::to_string(dcb.BaudRate) +
        " bod; " + std::to_string(dcb.ByteSize) +
        " bytes; parity control: " +
        parity.at(dcb.Parity) +
        "; stop bits: " +
        stop_bits.at(dcb.StopBits)
    );
    captions.push_back("file name: " + protocol.getFileName());

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

    launchUpdater(cmd_params);

    flashUpdate(0, cmd_params);

    std::cout << "press 'enter' to exit..." << std::endl;
    getchar();

    return 0;
}
