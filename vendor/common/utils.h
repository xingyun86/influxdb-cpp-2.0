// utils.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>
#include <fstream>
#include <functional>
#include <ppsyqm/json.hpp>

#ifndef _MSC_VER
#include <unistd.h>
#include <limits.h>
#define MAX_PATH_LEN	PATH_MAX
#else
#include <winsock2.h>
#define MAX_PATH_LEN	MAX_PATH
#define popen   _popen
#define pclose   _pclose
#endif

class Utils {
public:
    static std::string RunCmd(const std::string& cmd)
    {
        std::string ret = ("");
        FILE* f = popen(cmd.c_str(), "r");
        if (f != nullptr)
        {
            size_t s = 0;
            char d[MAX_PATH_LEN] = { 0 };
            while (!feof(f))
            {
                s = fread(d, sizeof(char), sizeof(d) / sizeof(char), f);
                if (s > 0)
                {
                    ret.append(d, s);
                }
            }
            pclose(f);
        }
        else
        {
            printf("popen %s error:(%d),%s.\n", cmd.c_str(), errno, strerror(errno));
        }
        return ret;
    }

    static std::string AppDir() {
        std::string data(MAX_PATH_LEN, '\0');
#ifndef _MSC_VER
        char szTmp[64];
        sprintf(szTmp, "/proc/%d/exe", getpid());
        if (readlink(szTmp, (char*)data.data(), data.size()) <= 0) {
            return ("");
        }
#else
        GetModuleFileNameA(NULL, (LPSTR)data.data(), (DWORD)data.size());
        for (auto& it : data) { if (it == '\\') { it = '/'; } };
#endif
        return  data.substr(0, data.rfind('/'));
    }
public:
    template <typename ...Args>
    static std::string format_string(const char* format, Args... args) {
        char buffer[BUFSIZ] = {0};
        size_t newlen = snprintf(buffer, BUFSIZ, format, args...);
        if (newlen > BUFSIZ)
        {
            std::vector<char> newbuffer(newlen + 1);
            snprintf(newbuffer.data(), newlen, format, args...);
            return std::string(newbuffer.data());
        }
        return buffer;
    }
public:
    static void Load(const std::string& fname, std::function<void(ppsyqm::json&)> cb)
    {
        std::string str((std::istreambuf_iterator<char>(std::ifstream(fname, std::ios::in | std::ios::binary).rdbuf())), std::istreambuf_iterator<char>());
        if (str.empty())
        {
            str = "{}";
        }
        auto json_obj = ppsyqm::json::parse(str);
        if (json_obj.is_object())
        {
           cb(json_obj);
        }
    }
    static void Save(const std::string& fname, std::function<void(ppsyqm::json&)> cb)
    {
        std::ofstream of = {};
        std::string str((std::istreambuf_iterator<char>(std::ifstream(fname, std::ios::in | std::ios::binary).rdbuf())), std::istreambuf_iterator<char>());
        if (str.empty())
        {
            str = "{}";
        }
        auto json_obj = ppsyqm::json::parse(str);
        of.open(fname, std::ios::out);
        if (of.is_open()) {
            cb(json_obj);
            std::string dump = json_obj.dump(2);
            of.write(dump.data(), dump.size());
        }
    }
};