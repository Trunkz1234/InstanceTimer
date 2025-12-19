#pragma once

#define GWCA_VERSION_MAJOR 2
<<<<<<< Updated upstream
#define GWCA_VERSION_MINOR 0
#define GWCA_VERSION_PATCH 8
#define GWCA_VERSION_BUILD 0
#define GWCA_VERSION "2.0.8.0"
=======
#define GWCA_VERSION_MINOR 1
#define GWCA_VERSION_PATCH 0
#define GWCA_VERSION_BUILD 0
#define GWCA_VERSION "2.1.0.0"
>>>>>>> Stashed changes

namespace GWCA {
    constexpr int VersionMajor = GWCA_VERSION_MAJOR;
    constexpr int VersionMinor = GWCA_VERSION_MINOR;
    constexpr int VersionPatch = GWCA_VERSION_PATCH;
    constexpr int VersionBuild = GWCA_VERSION_BUILD;
    constexpr const char* Version = GWCA_VERSION;
}
