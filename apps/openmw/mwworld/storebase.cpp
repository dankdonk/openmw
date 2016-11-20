#include "storebase.hpp"

namespace MWWorld
{
    RecordId::RecordId(const std::string &id, bool isDeleted)
        : mId(id), mIsDeleted(isDeleted)
    {}
}
