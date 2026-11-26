#ifndef ERLHUB_INHERITANCEACCESSDEMO_H
#define ERLHUB_INHERITANCEACCESSDEMO_H

#include <string>
#include <type_traits>

// Supporting example for CIC-211 Unit III: inheritance access modes.
// This is intentionally NOT part of the relief-vehicle hierarchy.
namespace ReliefOps {
namespace Demos {

// Base class with one member at each access level.
class AccessBase {
public:
    explicit AccessBase(int tag, int publicValue, int protectedValue,
                        int privateValue);

    int tag() const { return tag_; }
    int publicValue() const { return publicValue_; }

    // Public gate to the private value: derived classes cannot touch privateValue_
    // directly, but they (and outside code) can call this public function.
    int privateValueThroughPublicApi() const { return privateValue_; }

protected:
    int protectedValue() const { return protectedValue_; }
    void setProtectedValue(int value) { protectedValue_ = value; }

private:
    int tag_;
    int publicValue_;
    int protectedValue_;
    int privateValue_;
};

// PUBLIC inheritance: public base members stay public, protected stay protected,
// private stay inaccessible. Outside code can convert PubliclyDerived* to AccessBase*.
class PubliclyDerived : public AccessBase {
public:
    using AccessBase::AccessBase;
    int accessibleTotal() const { return publicValue() + protectedValue(); }
};

// PRIVATE inheritance: public and protected base members become private members of
// the derived class. They are usable inside PrivatelyDerived only; outside code and
// further descendants cannot convert to AccessBase* or call them.
class PrivatelyDerived : private AccessBase {
public:
    explicit PrivatelyDerived(int tag, int pub, int prot, int priv);
    int accessibleTotal() const { return publicValue() + protectedValue(); }
    int exposedTag() const { return tag(); }

protected:
    int baseProtectedForDescendants() const { return protectedValue(); }
};

// PROTECTED inheritance: public and protected base members become protected in the
// derived class. Outside code cannot convert to AccessBase*, but descendants can
// still reach them.
class ProtectedlyDerived : protected AccessBase {
public:
    explicit ProtectedlyDerived(int tag, int pub, int prot, int priv);
    int accessibleTotal() const { return publicValue() + protectedValue(); }
    int exposedPrivateValue() const { return privateValueThroughPublicApi(); }

protected:
    int basePublicForDescendants() const { return publicValue(); }
    int baseProtectedForDescendants() const { return protectedValue(); }
};

// A grandchild proves that protected inheritance keeps access alive down the chain.
class PublicGrandchild : public ProtectedlyDerived {
public:
    explicit PublicGrandchild(int tag, int pub, int prot, int priv);
    int grandchildAccessibleTotal() const {
        return basePublicForDescendants() + baseProtectedForDescendants();
    }
};

// Compile-time proof of the three conversion rules.
static_assert(std::is_convertible<PubliclyDerived*, AccessBase*>::value,
              "public inheritance permits implicit upcasting");
static_assert(!std::is_convertible<ProtectedlyDerived*, AccessBase*>::value,
              "protected inheritance blocks outside upcasting");
static_assert(!std::is_convertible<PrivatelyDerived*, AccessBase*>::value,
              "private inheritance blocks outside upcasting");

std::string inheritanceAccessReport();

}  // namespace Demos
}  // namespace ReliefOps

#endif  // ERLHUB_INHERITANCEACCESSDEMO_H
