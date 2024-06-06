#pragma once
#include "clients/concrete/LivoxClient.h"
#include <string>
namespace mandeye
{
bool saveLaz(const std::string& filename, const LivoxPointsBufferPtr& buffer);
}