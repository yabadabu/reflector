#include "reflector/reflector.h"

namespace Reflector {

  template< typename T>
  static void basic_to_binary(Buffer& b, Ref r) {
    T* v = r.as<T>();
    b.writePOD(*v);
  }
  template< typename T>
  static void binary_to_basic(BinParser& b, Ref r) {
    b.readBytes((void*)r.rawAddr(), sizeof(T));
  }

  void registerBinaryIOCommonTypes() {
    reflect<float>("float", binaryIO{ &basic_to_binary<float>, &binary_to_basic<float> });
    reflect<int>("int", binaryIO{ &basic_to_binary<int>, &binary_to_basic<int> });
  }

}
