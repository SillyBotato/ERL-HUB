#include "LanguageFeaturesDemo.h"

#include <iterator>
#include <memory>
#include <sstream>

#include "Types.h"

namespace ReliefOps {
namespace Demos {

std::string languageFeaturesReport() {
    using ReliefOps::Inventory::Priority;
    using ReliefOps::Inventory::ReliefCargo;
    using ReliefOps::Inventory::SupplyCategory;

    // ARRAY OF OBJECTS: a fixed-size stack array of ReliefCargo values
    // (contrast with TransportManifest's dynamic array of pointers).
    ReliefCargo cargoArray[3] = {
        ReliefCargo("L1", "Rations", SupplyCategory::Rations, 2, 1.0, 0.2,
                    Priority::Routine),
        ReliefCargo("L2", "Water", SupplyCategory::Water, 3, 2.0, 0.3,
                    Priority::Urgent),
        ReliefCargo("L3", "Blankets", SupplyCategory::Blankets, 5, 1.5, 0.4,
                    Priority::Routine)};

    const double totalQuantity = totalOf(cargoArray, [](const ReliefCargo& cargo) {
        return cargo.quantity();
    });

    // ITERATORS: explicit begin/end traversal of the array.
    std::size_t iterated = 0;
    for (auto it = std::begin(cargoArray); it != std::end(cargoArray); ++it) {
        ++iterated;
    }

    // ALLOCATORS: allocate raw storage, construct objects in place, destroy
    // them and release memory through std::allocator / allocator_traits.
    std::allocator<ReliefCargo> allocator;
    ReliefCargo* storage = allocator.allocate(2);
    std::size_t constructed = 0;
    try {
        std::allocator_traits<std::allocator<ReliefCargo>>::construct(
            allocator, storage, cargoArray[0]);
        ++constructed;
        std::allocator_traits<std::allocator<ReliefCargo>>::construct(
            allocator, storage + 1, cargoArray[1]);
        ++constructed;
    } catch (...) {
        while (constructed > 0) {
            std::allocator_traits<std::allocator<ReliefCargo>>::destroy(
                allocator, storage + --constructed);
        }
        allocator.deallocate(storage, 2);
        throw;
    }

    // Explicit + implicit type conversions and the friend function.
    const Litres explicitLitres(20.0);  // explicit double -> Litres
    const double implicitDouble = explicitLitres;  // implicit Litres -> double
    const bool friendResult =
        sameVolume(Litres(50.0), Litres(50.0));

    std::ostringstream oss;
    oss << "Language features demo\n"
        << "  array of objects: 3 cargo lines, total quantity " << totalQuantity
        << '\n'
        << "  iterators traversed: " << iterated << " elements\n"
        << "  allocator constructed " << constructed << " cargo objects\n"
        << "  explicit ctor / implicit conversion: Litres(20) -> "
        << implicitDouble << " L\n"
        << "  friend function sameVolume(50, 50): "
        << (friendResult ? "true" : "false") << '\n';

    while (constructed > 0) {
        std::allocator_traits<std::allocator<ReliefCargo>>::destroy(
            allocator, storage + --constructed);
    }
    allocator.deallocate(storage, 2);
    return oss.str();
}

}  // namespace Demos
}  // namespace ReliefOps
