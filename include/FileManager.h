#ifndef ERLHUB_FILEMANAGER_H
#define ERLHUB_FILEMANAGER_H

#include <cstdint>
#include <string>

#include "Records.h"

// UNIT IV: binary persistence with field-by-field serialisation, seekp()/seekg()
// and transactional replacement of the destination file.
namespace ReliefOps {
namespace Logistics {

class FileManager {
public:
    explicit FileManager(std::string path);

    const std::string& path() const { return path_; }

    // Returns false if the file is missing or cannot be read.
    bool load(HubSnapshot& snapshot) const;

    // Writes to a temporary file, verifies it, then atomically replaces the target.
    bool save(const HubSnapshot& snapshot) const;

    static constexpr std::uint32_t VERSION = 1;
    static constexpr std::size_t HEADER_SIZE = 20;

private:
    std::string path_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_FILEMANAGER_H
