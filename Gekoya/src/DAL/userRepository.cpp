#include "UserRepository.h"
#include "../DAL/Database.h"
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/collection.hpp>

bool UserRepository::InsertUser(const std::string& username, const std::string& password, int accessLevel)
{
    try
    {
        printf("InsertUser called: %s / %s / %d\n", username.c_str(), password.c_str(), accessLevel);
        auto db = Database::Get().GetDB();
        auto collection = db["users"];
        bsoncxx::builder::basic::document doc{};
        doc.append(bsoncxx::builder::basic::kvp("username", username.data()));
        doc.append(bsoncxx::builder::basic::kvp("password", password.data()));
        doc.append(bsoncxx::builder::basic::kvp("accessLevel", accessLevel));
        auto result = collection.insert_one(doc.view());
        return result ? true : false;
    }
    catch (const std::exception& e)
    {
        printf("InsertUser error: %s\n", e.what());
        return false;
    }
}

bool UserRepository::UserExists(const std::string& username)
{
    try
    {
        auto db = Database::Get().GetDB();
        auto collection = db["users"];
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("username", username.data()));
        auto result = collection.find_one(filter.view());
        return result ? true : false;
    }
    catch (const std::exception& e)
    {
        printf("UserExists error: %s\n", e.what());
        return false;
    }
}

bool UserRepository::ValidateUser(const std::string& username, const std::string& password)
{
    try
    {
        auto db = Database::Get().GetDB();
        auto collection = db["users"];
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("username", username.data()));
        filter.append(bsoncxx::builder::basic::kvp("password", password.data()));
        printf("ValidateUser searching: %s / %s\n", username.c_str(), password.c_str());
        auto result = collection.find_one(filter.view());
        printf("ValidateUser result: %s\n", result ? "FOUND" : "NOT FOUND");
        return result ? true : false;
    }
    catch (const std::exception& e)
    {
        printf("ValidateUser error: %s\n", e.what());
        return false;
    }
}

int UserRepository::GetUserAccessLevel(const std::string& username)
{
    try
    {
        auto db = Database::Get().GetDB();
        auto collection = db["users"];
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("username", username.data()));
        auto result = collection.find_one(filter.view());
        if (result)
            return result->view()["accessLevel"].get_int32().value;
        return 1;
    }
    catch (const std::exception& e)
    {
        printf("GetUserAccessLevel error: %s\n", e.what());
        return 1;
    }
}