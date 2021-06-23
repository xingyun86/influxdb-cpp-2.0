// testdemo.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <thread>
#include <influxdb-cpp-2.0.hpp>

// TODO: Reference additional headers your program requires here.
class DbHelper {
public:
    DbHelper() {
#ifdef _MSC_VER
        WSADATA wsadata = { 0 };
        WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif
    }
    ~DbHelper() {
#ifdef _MSC_VER
        WSACleanup();
#endif
    }
public:
    class CRandomString {
    public:
        CRandomString() { srand((unsigned int)time(nullptr)); }
        virtual ~CRandomString() {}
    private:
        char _digit()
        {
            return static_cast<char>('0' + rand() % ('9' - '0' + 1));
        }
        char _lower()
        {
            return static_cast<char>('a' + rand() % ('z' - 'a' + 1));
        }
        char _upper()
        {
            return static_cast<char>('A' + rand() % ('Z' - 'A' + 1));
        }
    public:
        double D(int min, int max)
        {
            return static_cast<double>(min + rand() % (max - min + 1));
        }
        float f(int min, int max)
        {
            return static_cast<float>(min + rand() % (max - min + 1));
        }
        int N(int min, int max)
        {
            return static_cast<int>(min + rand() % (max - min + 1));
        }
        std::string make_random_string(int length) {
            std::string str = ("");
            while (length-- != 0)
            {
                switch (rand() % 3)
                {
                case 0:str.push_back(_digit()); break;
                case 1:str.push_back(_lower()); break;
                default:str.push_back(_upper()); break;
                }
            }
            return (str);
        }
    public:
        static CRandomString* Inst() {
            static CRandomString CRandomStringInstance;
            return &CRandomStringInstance;
        }
    };
    int do_test()
    {
        int ret = 0;
        std::string resp = ("");

        //influxdb_cpp::server_info si("127.0.0.1", 8086, "testx", "test", "test");
        influxdb_cpp_2::server_info si("192.168.1.241", 8086, "fgudb", "test", "12345678");
        // post_http demo with resp[optional]

        ret = influxdb_cpp_2::builder()
            .meas("test")
            .tag("k", "v")
            .tag("x", "y")
            .field("x", 10)
            .field("y", 10.3, 2)
            .field("b", !!10)
            .timestamp(1512722735522840439)
            .post_http(si, &resp);
        std::cout << ret << std::endl << resp << std::endl;

        // send_udp demo
        ret = influxdb_cpp_2::builder()
            .meas("test")
            .tag("k", "v")
            .tag("x", "y")
            .field("x", 10)
            .field("y", 3.14e18, 3)
            .field("b", !!10)
            .timestamp(1512722735522840439)
            .send_udp("127.0.0.1", 8089);

        std::cout << ret << std::endl;

        // query from table
        //influxdb_cpp::server_info si_new("127.0.0.1", 8086, "", "test", "test");
        influxdb_cpp_2::server_info si_new("192.168.1.241", 8086, "", "test", "12345678");
        influxdb_cpp_2::query(resp, "show databases", si);
        std::cout << resp << std::endl;

        // create_db
        influxdb_cpp_2::create_db(resp, "x", si_new);
        std::cout << resp << std::endl;

        return 0;
    }
    int do_test_v2()
    {
        int ret = 0;
        std::string resp = ("");
        std::string bucketName = "TestDb";
        auto token = "nJbdGyatBia7uxsiwzgw2nuviptWv9-YKfgQaIEz_tJdSwoq4xZT8nOhSmWDRgWLJ_AgO9xuUdf5_qKLiK81Gw==";
        influxdb_cpp_2::server_info_v2 siv2_new("192.168.1.241", 8086, "", "test", "c1dc0a8116573b91", token, "ns");

        // create bucket
        influxdb_cpp_2::create_db_v2(resp, bucketName, siv2_new);
        std::cout << resp << std::endl;

        influxdb_cpp_2::server_info_v2 siv2("192.168.1.241", 8086, bucketName, "test", "c1dc0a8116573b91", token, "ns");
        // insert data to table
        time_t t = time(nullptr) * 1000;
        time_t e = t + 120 * 1000;
        for (; t <= e; t += 1000)
        {
            // post_http_v2 demo with resp[optional]
            ret = influxdb_cpp_2::builder()
                .meas("testdata")
                .tag("tag1", "value1")
                .tag("tag2", "value2")
                .field("tx", CRandomString::Inst()->N(1, 10000))
                .field("ty", CRandomString::Inst()->D(1, 10000), 3)
                .field("b", !!10)
                .timestamp(t * 1000000)
                .post_http_v2(siv2, &resp);
            if (ret != 0)
            {
                std::cout << ret << std::endl << resp << std::endl;
            }
        }

        //query from table
        influxdb_cpp_2::query_v2(resp, R"(from(bucket: \")" + bucketName + R"(\")|> range(start:-1h)|>filter(fn: (r)=> r[\"_measurement\"] == \"testdata\")|> filter(fn: (r) => r[\"_field\"] == \"tx\")|> yield())", siv2);
        std::cout << resp << std::endl;

        return 0;
    }
};