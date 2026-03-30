#include "Database.h"

mongocxx::instance& Database::GetInstance()
{
    static mongocxx::instance instance{};
    return instance;
}

Database& Database::Get()
{
    GetInstance(); // Must be called first!
    static Database db;
    return db;
}

Database::Database()
{
    try
    {
        mongocxx::uri uri{ "mongodb+srv://kpozhkata_db_user:2kLti8B9KGlUCF2m@gekoya.acittyu.mongodb.net/?appName=Gekoya" };
        client = mongocxx::client{ uri };
        printf("MongoDB connected successfully!\n");
    }
    catch (const std::exception& e)
    {
        printf("MongoDB error: %s\n", e.what());
    }
}

mongocxx::database Database::GetDB()
{
    return client["gekoya_db"];
}