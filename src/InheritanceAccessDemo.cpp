#include "InheritanceAccessDemo.h"

#include <sstream>

namespace ReliefOps {
namespace Demos {

AccessBase::AccessBase(int tag, int publicValue, int protectedValue,
                       int privateValue)
    : tag_(tag), publicValue_(publicValue), protectedValue_(protectedValue),
      privateValue_(privateValue) {}

PrivatelyDerived::PrivatelyDerived(int tag, int pub, int prot, int priv)
    : AccessBase(tag, pub, prot, priv) {}

ProtectedlyDerived::ProtectedlyDerived(int tag, int pub, int prot, int priv)
    : AccessBase(tag, pub, prot, priv) {}

PublicGrandchild::PublicGrandchild(int tag, int pub, int prot, int priv)
    : ProtectedlyDerived(tag, pub, prot, priv) {}

std::string inheritanceAccessReport() {
    PubliclyDerived pub(1, 10, 20, 30);
    PrivatelyDerived priv(2, 10, 20, 30);
    ProtectedlyDerived prot(3, 10, 20, 30);
    PublicGrandchild grand(4, 10, 20, 30);

    std::ostringstream oss;
    oss << "Inheritance access modes\n"
        << "  public    : derived total " << pub.accessibleTotal()
        << ", outside upcast allowed, private via API "
        << pub.privateValueThroughPublicApi() << '\n'
        << "  private   : derived total " << priv.accessibleTotal()
        << ", outside upcast blocked, tag via derived API " << priv.exposedTag()
        << '\n'
        << "  protected : derived total " << prot.accessibleTotal()
        << ", outside upcast blocked, private via API "
        << prot.exposedPrivateValue() << '\n'
        << "  grandchild of protected base: " << grand.grandchildAccessibleTotal()
        << '\n';
    return oss.str();
}

}  // namespace Demos
}  // namespace ReliefOps
