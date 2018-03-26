#include "xmodem_protocol.h"


XModem::XModem()
        : pack_num(1), default_crc(CRC::CRC_8) {
}


XModem::XModem(CRC crc_type)
        : pack_num(1), default_crc(crc_type) {
}


XModem::XModem(const std::vector<uint8_t> data, CRC crc_type)
        : pack_num(1), default_crc(crc_type), raw_data(data) {
}


XModem::~XModem() {
    err.clearAllErrors();
    std::vector<uint8_t>().swap(raw_data);
    std::vector<std::vector<uint8_t>>().swap(file_data);
    delete port;
}


std::vector<uint8_t> XModem::wrapData(HeaderBytes first_byte) {
    std::vector<uint8_t> send_data;

    send_data.push_back(first_byte);
    send_data.push_back(pack_num);
    send_data.push_back(0xFF - pack_num);
    pack_num++;

    for (auto i : raw_data) {
        send_data.push_back(i);
    }

    std::vector<uint8_t> crc = evalCRC(raw_data, default_crc);

    for (auto i : crc) {
        send_data.push_back(i);
    }

    return send_data;
}


void XModem::setDefaultCrcType(const CRC crc_type) {
    default_crc = crc_type;
}


void XModem::openPort(const SPortParams port_params) {
    port = new CPort(port_params);
}


void XModem::setDataBlock(std::vector<uint8_t> &data) {
    raw_data = data;
}


uint32_t XModem::sendPack(std::vector<uint8_t> &data, const Protocol protocol, const uint8_t clear_bufer) {
    if (((protocol == Protocol::PR_XMODEM) &&
            (data.size() != PackSize::XMODEM_SIZE)) ||
            ((protocol == Protocol::PR_XMODEM_1K) &&
            (data.size() != PackSize::XMODEM_1K_SIZE))) {
        err.addError("ошибка: несовпадение размера пакета с указанным в протоколе...");
        return 0;
    }

    setDataBlock(data);

    std::vector<uint8_t> send_data;

    switch (protocol) {
        case Protocol::PR_XMODEM:
            send_data = wrapData(HeaderBytes::SOH);
            break;
        case Protocol::PR_XMODEM_1K:
            send_data = wrapData(HeaderBytes::STX);
            break;
        case Protocol::PR_NONE:
        default:
            break;
    }

    port->sendDataPack<uint8_t>(send_data, clear_bufer);

    return 1;
}


void XModem::cancelTransmission() {
    std::vector<uint8_t> send_data = wrapData(HeaderBytes::EOT);
    // послать данные
}


void XModem::restartTransmission() {
    pack_num = 1;
    port->clearInputBuffer();
}


DCB XModem::getDCB() {
    return port->getParams().dcb;
}


std::string XModem::getPortName() {
    return port->getParams().port_name;
}


void XModem::setFileName(const std::string &file_name) {
    this->file_name = file_name;
}


std::string XModem::getFileName() {
    return file_name;
}


uint32_t XModem::loadFile(const Protocol protocol) {
    std::ifstream h_file(file_name, std::ios::binary);

    if (!h_file.is_open()) {
        err.addError("ошибка: не удалось открыть файл...");
        return 0;
    }

    uint32_t pack_size;

    switch (protocol) {
        case Protocol::PR_XMODEM:
            pack_size = PackSize::XMODEM_SIZE;
            break;
        case Protocol::PR_XMODEM_1K:
            pack_size = PackSize::XMODEM_1K_SIZE;
            // так надо
            default_crc = CRC::CRC_16;
            break;
        case Protocol::PR_NONE:
        default:
            pack_size = PackSize::UNLIMITED_SIZE;
            break;
    }

    std::vector<std::vector<uint8_t>>().swap(file_data);

    file_data.push_back(std::vector<uint8_t>());

    while (!h_file.eof()) {
        uint8_t data_byte;

        h_file.read(reinterpret_cast<char*>(&data_byte), sizeof(data_byte));
        if (file_data.at(file_data.size() - 1).size() == pack_size) {
            file_data.push_back(std::vector<uint8_t>());
        }
        file_data.at(file_data.size() - 1).push_back(data_byte);
    }
    h_file.close();

    auto start_pos = file_data.at(file_data.size() - 1).size();

    for (auto i = start_pos; i < pack_size; ++i) {
        file_data.at(file_data.size() - 1).push_back(ServiceCmds::CTRL_Z);
    }

    return 1;
}


void XModem::terminalOutput(uint8_t byte) {
    std::vector<std::string> got_bytes(256);

    for (size_t i = 0; i < got_bytes.size(); ++i) {
        got_bytes.at(i) = static_cast<uint8_t>(i);
    }
    got_bytes.at(ServiceCmds::ACK) = " ACK\n";
    got_bytes.at(ServiceCmds::C) = " C\n";
    got_bytes.at(ServiceCmds::CTRL_Z) = " CTRL_Z\n";
    got_bytes.at(ServiceCmds::NAK) = " NAK\n";
    got_bytes.at(HeaderBytes::CAN) = " CAN\n";
    got_bytes.at(HeaderBytes::EOT) = " EOT\n";
    got_bytes.at(HeaderBytes::ETB) = " ETB\n";
    got_bytes.at(HeaderBytes::SOH) = " SOH\n";
    got_bytes.at(HeaderBytes::STX) = " STX\n";

    std::cout << got_bytes.at(byte);
}


uint32_t XModem::transmitFile(const Protocol protocol) {
    if (!loadFile(protocol)) {
        std::cout << err.getLastError() << std::endl;
        return 0;
    }

    port->clearInputBuffer();

    std::cout << std::endl << "готов к передаче файла..." << std::endl;

    uint32_t trans_started = 0, wait_for_resp = 0, finishing_job = 0, all_done = 0;
    size_t packs_sent = 0;

    while (1) {
        uint8_t got_one = 0;
        uint8_t res = port->readOneSymbol<uint8_t>(got_one);

        if (got_one) {
            terminalOutput(res);
        }

        if (!all_done) {
            if (got_one) {
                if (ServiceCmds::C == res) {
                    trans_started = 1;
                    wait_for_resp = 0;
                    finishing_job = 0;
                    packs_sent = 0;
                    restartTransmission();
                    std::cout << " передача началась..." << std::endl;
                } else if (wait_for_resp) {
                    if (ServiceCmds::ACK == res) {
                        if (finishing_job) {
                            wait_for_resp = 0;
                            finishing_job = 0;
                            all_done = 1;
                            std::cout << "готово" << std::endl;
                        } else {
                            wait_for_resp = 0;
                            packs_sent++;
                            std::cout << " " << packs_sent << " пакетов из " << file_data.size() << " отправлено..." << std::endl;
                            if (file_data.size() == packs_sent) {
                                std::cout << " завершение..." << std::endl;
                                finishing_job = 1;
                                trans_started = 0;
                            }
                        }
                    } else if (ServiceCmds::NAK == res) {
                        wait_for_resp = 0;
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if ((trans_started) && (!wait_for_resp)) {
                port->clearInputBuffer();
                sendPack(file_data.at(packs_sent));
                wait_for_resp = 1;
            }
            if ((finishing_job) && (!wait_for_resp)) {
                std::vector<uint8_t> finish_byte{ HeaderBytes::EOT };

                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                port->clearInputBuffer();
                port->sendDataPack<uint8_t>(finish_byte);
                std::cout << " байт EOT отправлен..." << std::endl;
                wait_for_resp = 1;
            }
        }

        if ((!trans_started) && (!finishing_job)) {
            if (_kbhit()) {
                uint8_t key = _getch();

                if (VK_ESCAPE == key) {
                    if (!all_done) {
                        std::cout << "передача файла отменена" << std::endl;
                        all_done = 1;
                    }
                } else if (VK_RETURN == key) {
                    std::cout << std::endl;
                } else {
                    std::cout << key;
                }
                port->sendDataPack<uint8_t>(std::vector<uint8_t>{key});
            }
        }
        if ((_kbhit()) && (!all_done)) {
            uint8_t key = _getch();

            if (VK_ESCAPE == key) {
                std::cout << "передача файла отменена" << std::endl;
                all_done = 1;
                trans_started = 0;
                finishing_job = 0;

                std::vector<uint8_t> finish_byte{ HeaderBytes::CAN };

                port->clearInputBuffer();
                port->sendDataPack<uint8_t>(finish_byte);
                finish_byte.at(0) = HeaderBytes::EOT;
                port->sendDataPack<uint8_t>(finish_byte);
            }
        }
        if (all_done) {
            return 1;
        }
    }

    return 1;
}


uint32_t XModem::startTerminalMode() {
    std::cout << "режим терминала..." << std::endl;

    while (1) {
        uint8_t got_one = 0;
        uint8_t res = port->readOneSymbol<uint8_t>(got_one);

        if (got_one) {
            terminalOutput(res);
        }

        if (_kbhit()) {
            uint8_t key = _getch();

            if (VK_ESCAPE == key) {
                return 1;
            } else if (VK_RETURN == key) {
                std::cout << std::endl;
            } else {
                std::cout << key;
            }

            port->sendDataPack<uint8_t>(std::vector<uint8_t>{key});
        }
    }

    return 1;
}
