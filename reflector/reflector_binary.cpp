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

  static void string_to_binary(Buffer& b, Ref r) {
    std::string* v = r.as<std::string>();
    b.writePOD(v->size());
    b.writeBytes(v->data(), v->size());
  }

  static void binary_to_string(BinParser& b, Ref r) {
    std::string* v = r.as<std::string>();
    std::string::size_type sz;
    b.readPOD(sz);
    const void* data = b.consume(sz);
    *v = std::string((const char*)data, sz);
  }

  void registerBinaryIOCommonTypes() {
    reflect<binaryIO>("binaryIO" );
    reflect<float>("float", binaryIO{ &basic_to_binary<float>, &binary_to_basic<float> });
    reflect<int>("int", binaryIO{ &basic_to_binary<int>, &binary_to_basic<int> });
    reflect<bool>("bool", binaryIO{ &basic_to_binary<char>, &binary_to_basic<char> });
    reflect<uint32_t>("u32", binaryIO{ &basic_to_binary<uint32_t>, &binary_to_basic<uint32_t> });
    reflect<uint64_t>("u64", binaryIO{ &basic_to_binary<uint64_t>, &binary_to_basic<uint64_t> });
    reflect<uint8_t>("u8", binaryIO{ &basic_to_binary<uint8_t>, &binary_to_basic<uint8_t> });
    reflect<std::string>("std::string", binaryIO{ &string_to_binary, &binary_to_string });
  }

}
