/*
 * @File	  : self_exception.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/05 21:15
 * @Introduce : �ļ��쳣��
*/

#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

#ifndef SELF_EXCEPTION_HPP
#define SELF_EXCEPTION_HPP  

namespace self {
    class FileException : public std::exception {
    private:
        const char* msg{ "File Exception" };
    public:
        FileException(const char* msg) {
            this->msg = msg;
        }

        const char* getMessage() {
            return msg;
        }

        virtual const char* what() const throw() {
            return msg;
        }
    };

    class TimeoutException : public std::runtime_error {
    private:
        const char* msg{};
    public:
        TimeoutException(const char* msg = "Timeout Exception") : std::runtime_error(msg) {
            this->msg = msg;
        };

        const char* getMessage() {
            return msg;
        }

        virtual const char* what() const throw() override {
            return msg;
        }
    };

    class HTTPException : public std::runtime_error {
    private:
        std::string msg{};
        unsigned short code{ 500 };
        json extra{};
    public:
        HTTPException(std::string_view msg = "Severe HTTP Error", unsigned short code = 500, const json& extra = json()) : std::runtime_error(msg.data()) {
            this->msg = msg;
            this->code = code;
            this->extra = extra;
        };

        HTTPException(unsigned short code) : HTTPException("", code) { };

        HTTPException(unsigned short code, const std::string& msg) : HTTPException(msg, code) { };

        HTTPException(unsigned short code, const json& extra) : HTTPException("", code, extra) { };

        const std::string& getMessage() const {
            return this->msg;
        }

        const unsigned short getCode() const {
            return this->code;
        }
        
        json getJson() const {
            return this->extra;
        }

        virtual const char* what() const throw() override {
            return this->msg.data();
        }
    };
};

#endif