#pragma once


#include <string>
#include <vector>


class Errors {
    public:
        ~Errors();
        void addError(const std::string &error);
        std::string getLastError();
        size_t getErrorsNum();
        void clearAllErrors();
    private:
        std::vector<std::string> errors;
};
