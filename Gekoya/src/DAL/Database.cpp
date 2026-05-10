#include "Database.h"
#include <mongocxx/exception/exception.hpp>

static mongocxx::instance global_instance{};

Database& Database::Get()
{
    static Database database;
    return database;
}

Database::Database()
{
    try
    {
        mongocxx::uri uri{ "mongodb://kpozhkata_db_user:2kLti8B9KGlUCF2m@ac-0rfaeiz-shard-00-00.acittyu.mongodb.net:27017,ac-0rfaeiz-shard-00-01.acittyu.mongodb.net:27017,ac-0rfaeiz-shard-00-02.acittyu.mongodb.net:27017/?ssl=true&replicaSet=atlas-cnd41f-shard-0&authSource=admin&appName=Gekoya" };
        client = std::make_unique<mongocxx::client>(uri);
        db = std::make_unique<mongocxx::database>((*client)["gekoya_db"]);
        printf("MongoDB connected successfully!\n");
    }
    catch (const mongocxx::exception& e)
    {
        printf("MongoDB error: %s (code %d)\n", e.what(), e.code().value());
    }
    catch (const std::exception& e)
    {
        printf("MongoDB error: %s\n", e.what());
    }
}

mongocxx::database& Database::GetDB()
{
    return *db;
}