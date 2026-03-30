#pragma once
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>
#include <string>

class Database
{
public:
    static Database& Get();
    mongocxx::database GetDB();

private:
    static mongocxx::instance& GetInstance();
    Database();
    mongocxx::client client{};
};