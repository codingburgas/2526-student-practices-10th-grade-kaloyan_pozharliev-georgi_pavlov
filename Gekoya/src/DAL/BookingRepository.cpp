#include "BookingRepository.h"
#include "Database.h"
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/cursor.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

bool BookingRepository::InsertBooking(const Booking& booking)
{
    try
    {
        auto& db         = Database::Get().GetDB();
        auto  collection = db["bookings"];

        bsoncxx::builder::basic::document doc{};
        doc.append(kvp("username",    booking.username));
        doc.append(kvp("movieTitle",  booking.movieTitle));
        doc.append(kvp("showtime",    booking.showtime));
        doc.append(kvp("seatType",    booking.seatType));
        doc.append(kvp("bookingDate", booking.bookingDate));
        doc.append(kvp("price",       booking.price));

        auto result = collection.insert_one(doc.view());
        return result ? true : false;
    }
    catch (const std::exception& e)
    {
        printf("InsertBooking error: %s\n", e.what());
        return false;
    }
}

std::vector<Booking> BookingRepository::GetBookingsByUser(const std::string& username)
{
    std::vector<Booking> bookings;
    try
    {
        auto& db         = Database::Get().GetDB();
        auto  collection = db["bookings"];

        bsoncxx::builder::basic::document filter{};
        filter.append(kvp("username", username));

        auto cursor = collection.find(filter.view());
        for (auto& doc : cursor)
        {
            Booking b;
            // _id as hex string
            b.id          = doc["_id"].get_oid().value.to_string();
            b.username    = std::string(doc["username"].get_string().value);
            b.movieTitle  = std::string(doc["movieTitle"].get_string().value);
            b.showtime    = std::string(doc["showtime"].get_string().value);
            b.seatType    = std::string(doc["seatType"].get_string().value);
            b.bookingDate = std::string(doc["bookingDate"].get_string().value);
            b.price       = doc["price"].get_int32().value;
            bookings.push_back(b);
        }
    }
    catch (const std::exception& e)
    {
        printf("GetBookingsByUser error: %s\n", e.what());
    }
    return bookings;
}

bool BookingRepository::DeleteBooking(const std::string& id)
{
    try
    {
        auto& db         = Database::Get().GetDB();
        auto  collection = db["bookings"];

        bsoncxx::oid oid{ id };
        auto filter = make_document(kvp("_id", oid));
        auto result = collection.delete_one(filter.view());
        return result && result->deleted_count() > 0;
    }
    catch (const std::exception& e)
    {
        printf("DeleteBooking error: %s\n", e.what());
        return false;
    }
}
