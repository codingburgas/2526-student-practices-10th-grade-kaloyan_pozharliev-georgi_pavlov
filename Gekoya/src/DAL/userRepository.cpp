#include "UserRepository.h"
#include "../DAL/Database.h"
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/collection.hpp>

bool UserRepository::InsertUser(const std::string& username, const std::string& password, const std::string& email, int accessLevel)
{
    try
    {
        printf("InsertUser called: %s / %s / %s / %d\n", username.c_str(), password.c_str(), email.c_str(), accessLevel);
        auto& db = Database::Get().GetDB();
        auto  collection = db["users"];

        bsoncxx::builder::basic::document doc{};
        doc.append(bsoncxx::builder::basic::kvp("username", username));
        doc.append(bsoncxx::builder::basic::kvp("password", password));
        doc.append(bsoncxx::builder::basic::kvp("email", email));
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
        auto& db = Database::Get().GetDB();
        auto  collection = db["users"];

        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("username", username));

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
        auto& db = Database::Get().GetDB();
        auto  collection = db["users"];

        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("username", username));
        filter.append(bsoncxx::builder::basic::kvp("password", password));

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
        auto& db = Database::Get().GetDB();
        auto  collection = db["users"];

        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("username", username));

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

std::string UserRepository::GetUserEmail(const std::string& username)
{
    try
    {
        auto& db = Database::Get().GetDB();
        auto  collection = db["users"];

        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("username", username));

        auto result = collection.find_one(filter.view());
        if (result)
        {
            auto view = result->view();
            if (view.find("email") != view.end())
                return std::string(view["email"].get_string().value);
        }
        return "";
    }
    catch (const std::exception& e)
    {
        printf("GetUserEmail error: %s\n", e.what());
        return "";
    }
}

bool UserRepository::UpdateUser(const std::string& username, const std::string& newEmail, const std::string& newPassword)
{
    try
    {
        auto& db = Database::Get().GetDB();
        auto  collection = db["users"];

        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("username", username));

        bsoncxx::builder::basic::document setDoc{};
        bool hasUpdate = false;

        if (!newEmail.empty())
        {
            setDoc.append(bsoncxx::builder::basic::kvp("email", newEmail));
            hasUpdate = true;
        }
        if (!newPassword.empty())
        {
            setDoc.append(bsoncxx::builder::basic::kvp("password", newPassword));
            hasUpdate = true;
        }

        if (!hasUpdate)
            return true; // nothing to update, treat as success

        bsoncxx::builder::basic::document update{};
        update.append(bsoncxx::builder::basic::kvp("$set", setDoc));

        auto result = collection.update_one(filter.view(), update.view());
        return result && result->matched_count() > 0;
    }
    catch (const std::exception& e)
    {
        printf("UpdateUser error: %s\n", e.what());
        return false;
    }
}