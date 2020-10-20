#include "reflector/reflector_binary.h"
#include <string>

namespace REFLECTOR_NAMESPACE {

  void toBinary(Buffer& buf, Ref r) {
    BinEncoder b(buf);
    b.write(r);
    b.close();
  }

  void fromBinary(const Buffer& buf, Ref r) {
    BinDecoder b(buf);
    b.read(r);
  }

  template< typename T>
  static void pod_to_binary(BinEncoder& b, Ref r) {
    T* v = r.as<T>();
    b.writePOD(*v);
  }

  template< typename T>
  static void binary_to_pod(BinDecoder& b, Ref r) {
    b.readBytes((void*)r.rawAddr(), sizeof(T));
  }

  // Integers are encoded using a Varint encoding
  template< typename T>
  static void integer_to_binary(BinEncoder& b, Ref r) {
    T v = *r.as<T>();
    b.writeVarint(v);
  }

  // And read, then downcasted. We are safe as long as we don't change the schema
  template< typename T>
  static void binary_to_integer(BinDecoder& b, Ref r) {
    *(T*)r.rawAddr() = (T)b.readVarint();
  }

  // String prefix the length of the string followed by the string itself (without the \0)
  static void string_to_binary(BinEncoder& b, Ref r) {
    std::string* v = r.as<std::string>();
    b.writeVarint(v->size());
    b.writeBytes(v->data(), v->size());
  }

  static void binary_to_string(BinDecoder& b, Ref r) {
    std::string* v = r.as<std::string>();
    size_t sz = b.readVarint();
    const void* data = b.consume(sz);
    *v = std::string((const char*)data, sz);
  }

  void registerBinaryIOCommonTypes() {
    reflect<binaryIO>("binaryIO" );
    reflect<float>("float", binaryIO{ &pod_to_binary<float>, &binary_to_pod<float> });
    reflect<int>("int", binaryIO{ &integer_to_binary<int>, &binary_to_integer<int> });
    reflect<bool>("bool", binaryIO{ &pod_to_binary<char>, &binary_to_pod<char> });
    reflect<uint32_t>("u32", binaryIO{ &integer_to_binary<uint32_t>, &binary_to_integer<uint32_t> });
    reflect<uint64_t>("u64", binaryIO{ &integer_to_binary<uint64_t>, &binary_to_integer<uint64_t> });
    reflect<uint8_t>("u8", binaryIO{ &pod_to_binary<uint8_t>, &binary_to_pod<uint8_t> });
    reflect<std::string>("std::string", binaryIO{ &string_to_binary, &binary_to_string });
  }

}
