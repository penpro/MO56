#pragma once

#include "Runtime/Launch/Resources/Version.h"

static_assert(ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 6,
        "This module is pinned to UE 5.6. If upgrading, audit SaveSubsystem delegates and MD5 usage.");
