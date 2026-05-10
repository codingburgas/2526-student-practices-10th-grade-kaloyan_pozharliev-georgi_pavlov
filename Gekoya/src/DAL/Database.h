#pragma once
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>
#include <memory>
#include <string>

class Database
{
public:
    static Database& Get();
    mongocxx::database& GetDB();

private:
    static mongocxx::instance& GetInstance();
    Database();
    std::unique_ptr<mongocxx::client>   client;
    std::unique_ptr<mongocxx::database> db;
};