#include "FileManager.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace ReliefOps {
namespace Logistics {
namespace {

constexpr char MAGIC[8] = {'E', 'R', 'L', 'H', 'U', 'B', '0', '1'};
constexpr std::streamoff COUNT_OFFSET = 12;  // after magic (8) + version (4)
constexpr std::uint32_t MAX_STRING_BYTES = 16U * 1024U * 1024U;

template <typename T>
void writePod(std::ostream& os, const T& value) {
    os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
bool readPod(std::istream& is, T& value) {
    is.read(reinterpret_cast<char*>(&value), sizeof(T));
    return static_cast<std::size_t>(is.gcount()) == sizeof(T);
}

void writeString(std::ostream& os, const std::string& value) {
    if (value.size() > MAX_STRING_BYTES) {
        throw std::runtime_error("String too long for binary record");
    }
    const std::uint32_t length = static_cast<std::uint32_t>(value.size());
    writePod(os, length);
    if (length > 0) {
        os.write(value.data(), length);
    }
}

bool readString(std::istream& is, std::string& value) {
    std::uint32_t length = 0;
    if (!readPod(is, length) || length > MAX_STRING_BYTES) {
        return false;
    }
    value.assign(length, '\0');
    if (length > 0) {
        is.read(&value[0], length);
        if (static_cast<std::uint32_t>(is.gcount()) != length) {
            return false;
        }
    }
    return true;
}

void writeCargo(std::ostream& os, const CargoLineRecord& cargo) {
    writeString(os, cargo.id);
    writeString(os, cargo.name);
    writePod(os, static_cast<std::int32_t>(cargo.category));
    writePod(os, static_cast<std::int32_t>(cargo.quantity));
    writePod(os, cargo.weightPerUnitKg);
    writePod(os, cargo.volumePerUnitM3);
    writePod(os, static_cast<std::int32_t>(cargo.priority));
    writeString(os, cargo.destination);
}

bool readCargo(std::istream& is, CargoLineRecord& cargo) {
    std::int32_t category = 0;
    std::int32_t quantity = 0;
    std::int32_t priority = 0;
    if (!readString(is, cargo.id) || !readString(is, cargo.name) ||
        !readPod(is, category) || !readPod(is, quantity) ||
        !readPod(is, cargo.weightPerUnitKg) ||
        !readPod(is, cargo.volumePerUnitM3) || !readPod(is, priority) ||
        !readString(is, cargo.destination)) {
        return false;
    }
    cargo.category = Inventory::categoryFromInt(static_cast<int>(category));
    cargo.quantity = static_cast<int>(quantity);
    cargo.priority = Inventory::priorityFromInt(static_cast<int>(priority));
    return true;
}

void writeDepot(std::ostream& os, const DepotRecord& depot) {
    writeString(os, depot.id);
    writeString(os, depot.name);
    writeString(os, depot.location);
    const std::uint64_t count = depot.stock.size();
    writePod(os, count);
    for (const CargoLineRecord& cargo : depot.stock) {
        writeCargo(os, cargo);
    }
}

bool readDepot(std::istream& is, DepotRecord& depot) {
    std::uint64_t count = 0;
    if (!readString(is, depot.id) || !readString(is, depot.name) ||
        !readString(is, depot.location) || !readPod(is, count) ||
        count > 100000) {
        return false;
    }
    depot.stock.clear();
    for (std::uint64_t i = 0; i < count; ++i) {
        CargoLineRecord cargo;
        if (!readCargo(is, cargo)) {
            return false;
        }
        depot.stock.push_back(cargo);
    }
    return true;
}

void writeVehicle(std::ostream& os, const VehicleRecord& vehicle) {
    writePod(os, static_cast<std::int32_t>(vehicle.kind));
    writeString(os, vehicle.id);
    writeString(os, vehicle.model);
    writeString(os, vehicle.destination);
    writePod(os, vehicle.fuelCapacity);
    writePod(os, vehicle.currentFuel);
    writePod(os, vehicle.maxPayloadKg);
    writePod(os, static_cast<std::int32_t>(vehicle.status));
    writeString(os, vehicle.beaconId);
    writePod(os, vehicle.latitude);
    writePod(os, vehicle.longitude);
    writePod(os, vehicle.beaconActive);
    writePod(os, static_cast<std::int32_t>(vehicle.axles));
    writePod(os, static_cast<std::int32_t>(vehicle.crewCount));
    writePod(os, vehicle.rotorCraft);
    writePod(os, vehicle.hullCapacityTonnes);
    writePod(os, vehicle.waterSpeedKnots);
    writePod(os, static_cast<std::int32_t>(vehicle.triageBeds));
    writeString(os, vehicle.lifeSupportId);
    writePod(os, static_cast<std::int32_t>(vehicle.oxygenUnits));
    writePod(os, vehicle.surgicalReady);
    writePod(os, vehicle.fuelRatePerKm);
    const std::uint64_t manifestCount = vehicle.manifest.size();
    writePod(os, manifestCount);
    for (const CargoLineRecord& cargo : vehicle.manifest) {
        writeCargo(os, cargo);
    }
}

bool readVehicle(std::istream& is, VehicleRecord& vehicle) {
    std::int32_t kind = 0;
    std::int32_t status = 0;
    std::int32_t axles = 0;
    std::int32_t crew = 0;
    std::int32_t beds = 0;
    std::int32_t oxygen = 0;
    std::uint64_t manifestCount = 0;
    if (!readPod(is, kind) || !readString(is, vehicle.id) ||
        !readString(is, vehicle.model) || !readString(is, vehicle.destination) ||
        !readPod(is, vehicle.fuelCapacity) || !readPod(is, vehicle.currentFuel) ||
        !readPod(is, vehicle.maxPayloadKg) || !readPod(is, status) ||
        !readString(is, vehicle.beaconId) || !readPod(is, vehicle.latitude) ||
        !readPod(is, vehicle.longitude) || !readPod(is, vehicle.beaconActive) ||
        !readPod(is, axles) || !readPod(is, crew) || !readPod(is, vehicle.rotorCraft) ||
        !readPod(is, vehicle.hullCapacityTonnes) ||
        !readPod(is, vehicle.waterSpeedKnots) || !readPod(is, beds) ||
        !readString(is, vehicle.lifeSupportId) || !readPod(is, oxygen) ||
        !readPod(is, vehicle.surgicalReady) || !readPod(is, vehicle.fuelRatePerKm) ||
        !readPod(is, manifestCount) || manifestCount > 100000) {
        return false;
    }
    vehicle.kind = static_cast<VehicleKind>(kind);
    vehicle.status = static_cast<VehicleStatus>(status);
    vehicle.axles = static_cast<int>(axles);
    vehicle.crewCount = static_cast<int>(crew);
    vehicle.triageBeds = static_cast<int>(beds);
    vehicle.oxygenUnits = static_cast<int>(oxygen);
    vehicle.manifest.clear();
    for (std::uint64_t i = 0; i < manifestCount; ++i) {
        CargoLineRecord cargo;
        if (!readCargo(is, cargo)) {
            return false;
        }
        vehicle.manifest.push_back(cargo);
    }
    return true;
}

void writeRequest(std::ostream& os, const DispatchRequest& request) {
    writeString(os, request.id);
    writeString(os, request.depotId);
    writeString(os, request.vehicleId);
    writeString(os, request.cargoId);
    writeString(os, request.cargoName);
    writeString(os, request.destination);
    writePod(os, static_cast<std::int32_t>(request.quantity));
    writePod(os, request.weightKg);
    writePod(os, request.volumeM3);
    writePod(os, request.fuelEstimateL);
    writePod(os, static_cast<std::int32_t>(request.priority));
    writePod(os, static_cast<std::int32_t>(request.corridor));
    writePod(os, request.triageApproved);
    writePod(os, request.sequence);
    writeString(os, request.createdAt);
}

bool readRequest(std::istream& is, DispatchRequest& request) {
    std::int32_t quantity = 0;
    std::int32_t priority = 0;
    std::int32_t corridor = 0;
    if (!readString(is, request.id) || !readString(is, request.depotId) ||
        !readString(is, request.vehicleId) || !readString(is, request.cargoId) ||
        !readString(is, request.cargoName) || !readString(is, request.destination) ||
        !readPod(is, quantity) || !readPod(is, request.weightKg) ||
        !readPod(is, request.volumeM3) || !readPod(is, request.fuelEstimateL) ||
        !readPod(is, priority) || !readPod(is, corridor) ||
        !readPod(is, request.triageApproved) || !readPod(is, request.sequence) ||
        !readString(is, request.createdAt)) {
        return false;
    }
    request.quantity = static_cast<int>(quantity);
    request.priority = static_cast<Inventory::Priority>(priority);
    request.corridor = static_cast<CorridorStatus>(corridor);
    return true;
}

void writeDispatch(std::ostream& os, const DispatchRecord& record) {
    writeString(os, record.id);
    writeString(os, record.timestamp);
    writeString(os, record.depotId);
    writeString(os, record.vehicleId);
    writeString(os, record.cargoId);
    writeString(os, record.cargoName);
    writeString(os, record.destination);
    writePod(os, static_cast<std::int32_t>(record.quantity));
    writePod(os, record.weightKg);
    writePod(os, record.fuelUsedL);
    writePod(os, static_cast<std::int32_t>(record.priority));
    writeString(os, record.note);
}

bool readDispatch(std::istream& is, DispatchRecord& record) {
    std::int32_t quantity = 0;
    std::int32_t priority = 0;
    if (!readString(is, record.id) || !readString(is, record.timestamp) ||
        !readString(is, record.depotId) || !readString(is, record.vehicleId) ||
        !readString(is, record.cargoId) || !readString(is, record.cargoName) ||
        !readString(is, record.destination) || !readPod(is, quantity) ||
        !readPod(is, record.weightKg) || !readPod(is, record.fuelUsedL) ||
        !readPod(is, priority) || !readString(is, record.note)) {
        return false;
    }
    record.quantity = static_cast<int>(quantity);
    record.priority = static_cast<Inventory::Priority>(priority);
    return true;
}

void writeAudit(std::ostream& os, const AuditRecord& audit) {
    writeString(os, audit.id);
    writeString(os, audit.operation);
    writeString(os, audit.depotId);
    writeString(os, audit.vehicleId);
    writeString(os, audit.detail);
    writeString(os, audit.timestamp);
}

bool readAudit(std::istream& is, AuditRecord& audit) {
    return readString(is, audit.id) && readString(is, audit.operation) &&
           readString(is, audit.depotId) && readString(is, audit.vehicleId) &&
           readString(is, audit.detail) && readString(is, audit.timestamp);
}

void appendSection(std::ostream& os, std::uint8_t id, const std::string& payload) {
    writePod(os, id);
    const std::uint64_t size = payload.size();
    writePod(os, size);
    os.write(payload.data(), static_cast<std::streamsize>(size));
}

bool writeFile(const std::string& path, const HubSnapshot& snapshot) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out.write(MAGIC, sizeof(MAGIC));
    const std::uint32_t version = FileManager::VERSION;
    writePod(out, version);

    // Placeholder for the section count: patched later with seekp().
    const std::streampos countPosition = out.tellp();
    const std::uint64_t placeholder = 0;
    writePod(out, placeholder);

    std::ostringstream payload(std::ios::binary);
    writePod(payload, static_cast<std::uint64_t>(snapshot.depots.size()));
    for (const DepotRecord& depot : snapshot.depots) {
        writeDepot(payload, depot);
    }
    appendSection(out, 1, payload.str());

    payload.str(std::string());
    payload.clear();
    writePod(payload, static_cast<std::uint64_t>(snapshot.vehicles.size()));
    for (const VehicleRecord& vehicle : snapshot.vehicles) {
        writeVehicle(payload, vehicle);
    }
    appendSection(out, 2, payload.str());

    payload.str(std::string());
    payload.clear();
    writePod(payload, static_cast<std::uint64_t>(snapshot.history.size()));
    for (const DispatchRecord& record : snapshot.history) {
        writeDispatch(payload, record);
    }
    appendSection(out, 3, payload.str());

    payload.str(std::string());
    payload.clear();
    writePod(payload, static_cast<std::uint64_t>(snapshot.pending.size()));
    for (const DispatchRequest& request : snapshot.pending) {
        writeRequest(payload, request);
    }
    appendSection(out, 4, payload.str());

    payload.str(std::string());
    payload.clear();
    writePod(payload, static_cast<std::uint64_t>(snapshot.audits.size()));
    for (const AuditRecord& audit : snapshot.audits) {
        writeAudit(payload, audit);
    }
    appendSection(out, 5, payload.str());

    payload.str(std::string());
    payload.clear();
    writePod(payload, snapshot.totalDispatchedTonnes);
    writePod(payload, snapshot.nextSequence);
    appendSection(out, 6, payload.str());

    const std::uint64_t sectionCount = 6;
    out.seekp(countPosition, std::ios::beg);  // seekp demonstration
    writePod(out, sectionCount);
    out.seekp(0, std::ios::end);
    out.flush();
    return out.good();
}

}  // namespace

FileManager::FileManager(std::string path) : path_(std::move(path)) {}

bool FileManager::load(HubSnapshot& snapshot) const {
    std::ifstream in(path_, std::ios::binary);
    if (!in) {
        return false;
    }

    in.seekg(0, std::ios::beg);  // seekg demonstration
    char magic[8] = {};
    in.read(magic, sizeof(magic));
    if (!in || std::string(magic, sizeof(magic)) != std::string(MAGIC, sizeof(MAGIC))) {
        return false;
    }
    std::uint32_t version = 0;
    if (!readPod(in, version) || version != VERSION) {
        return false;
    }

    in.seekg(COUNT_OFFSET, std::ios::beg);  // seekg demonstration
    std::uint64_t sectionCount = 0;
    if (!readPod(in, sectionCount) || sectionCount > 1000) {
        return false;
    }
    in.seekg(HEADER_SIZE, std::ios::beg);

    HubSnapshot restored;
    for (std::uint64_t s = 0; s < sectionCount; ++s) {
        std::uint8_t sectionId = 0;
        std::uint64_t payloadSize = 0;
        if (!readPod(in, sectionId) || !readPod(in, payloadSize) ||
            payloadSize > MAX_STRING_BYTES) {
            return false;
        }
        std::string payload(static_cast<std::size_t>(payloadSize), '\0');
        if (payloadSize > 0) {
            in.read(&payload[0], static_cast<std::streamsize>(payloadSize));
            if (static_cast<std::uint64_t>(in.gcount()) != payloadSize) {
                return false;
            }
        }
        std::istringstream stream(payload, std::ios::binary);
        switch (sectionId) {
            case 1: {
                std::uint64_t count = 0;
                if (!readPod(stream, count) || count > 100000) return false;
                for (std::uint64_t i = 0; i < count; ++i) {
                    DepotRecord depot;
                    if (!readDepot(stream, depot)) return false;
                    restored.depots.push_back(depot);
                }
                break;
            }
            case 2: {
                std::uint64_t count = 0;
                if (!readPod(stream, count) || count > 100000) return false;
                for (std::uint64_t i = 0; i < count; ++i) {
                    VehicleRecord vehicle;
                    if (!readVehicle(stream, vehicle)) return false;
                    restored.vehicles.push_back(vehicle);
                }
                break;
            }
            case 3: {
                std::uint64_t count = 0;
                if (!readPod(stream, count) || count > 100000) return false;
                for (std::uint64_t i = 0; i < count; ++i) {
                    DispatchRecord record;
                    if (!readDispatch(stream, record)) return false;
                    restored.history.push_back(record);
                }
                break;
            }
            case 4: {
                std::uint64_t count = 0;
                if (!readPod(stream, count) || count > 100000) return false;
                for (std::uint64_t i = 0; i < count; ++i) {
                    DispatchRequest request;
                    if (!readRequest(stream, request)) return false;
                    restored.pending.push_back(request);
                }
                break;
            }
            case 5: {
                std::uint64_t count = 0;
                if (!readPod(stream, count) || count > 100000) return false;
                for (std::uint64_t i = 0; i < count; ++i) {
                    AuditRecord audit;
                    if (!readAudit(stream, audit)) return false;
                    restored.audits.push_back(audit);
                }
                break;
            }
            case 6:
                if (!readPod(stream, restored.totalDispatchedTonnes) ||
                    !readPod(stream, restored.nextSequence)) {
                    return false;
                }
                break;
            default:
                // Payload bytes were already consumed; seekg() above handles
                // forward navigation and the explicit header re-reads.
                break;
        }
    }
    if (!in) {
        return false;
    }
    snapshot = restored;
    return true;
}

bool FileManager::save(const HubSnapshot& snapshot) const {
    namespace fs = std::filesystem;
    const fs::path target(path_);
    const std::string temporary = path_ + ".tmp";
    const std::string backup = path_ + ".bak";
    std::error_code error;

    if (!writeFile(temporary, snapshot)) {
        fs::remove(temporary, error);
        return false;
    }

    // Verify the temporary file before touching the live data file.
    HubSnapshot verification;
    if (!FileManager(temporary).load(verification) ||
        verification.depots.size() != snapshot.depots.size() ||
        verification.vehicles.size() != snapshot.vehicles.size() ||
        verification.history.size() != snapshot.history.size()) {
        fs::remove(temporary, error);
        return false;
    }

    const bool hadOriginal = fs::exists(target, error);
    if (hadOriginal) {
        fs::remove(backup, error);
        fs::copy_file(target, backup, fs::copy_options::overwrite_existing, error);
        if (error) {
            fs::remove(temporary, error);
            return false;
        }
        fs::remove(target, error);
    }
    fs::rename(temporary, target, error);
    if (error) {
        if (hadOriginal) {
            std::error_code restoreError;
            fs::copy_file(backup, target, fs::copy_options::overwrite_existing,
                          restoreError);
        }
        fs::remove(temporary, error);
        return false;
    }
    return true;
}

}  // namespace Logistics
}  // namespace ReliefOps
