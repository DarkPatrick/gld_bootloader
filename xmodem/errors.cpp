#include "errors.h"


Errors::~Errors() {
    clearAllErrors();
}


void Errors::addError(const std::string &error) {
    errors.push_back(error);
}


std::string Errors::getLastError() {
    std::string ret_val = "";

    if (errors.size()) {
        ret_val = errors.back();
        errors.pop_back();
    }

    return ret_val;
}


size_t Errors::getErrorsNum() {
    return errors.size();
}


void Errors::clearAllErrors() {
    std::vector<std::string>().swap(errors);
}
