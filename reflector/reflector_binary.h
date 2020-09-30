#pragma once

#include <vector>

namespace Reflector {

  class Type;
  class Buffer;
  class BinParser;

  // -----------------------------------------------------------------------------------
  // Binary IO
  struct binaryIO {
    std::function<void(Buffer& b, Ref r)>       write;
    std::function<void(BinParser& b, Ref r)> read;
  };

  static constexpr uint32_t magic_header_types = 0x55114422;
  static constexpr uint32_t magic_struct_types = 0x55114422;
  using IndexType = uint32_t;

  // -----------------------------------------------------------------------------------
  class Buffer : public std::vector< uint8_t > {

    std::unordered_map< const Type*, IndexType > dict;
    bool closed = false;

  public:

    void rewind() {
      clear();
      closed = false;
    }

    void writeBytes(const void* addr, size_t nbytes) {
      assert(!closed);
      insert(end(), (uint8_t*)addr, (uint8_t*)addr + nbytes);
    }

    template< typename T >
    void writePOD(const T& t) {
      writeBytes(&t, sizeof(T));
    }

    void writeType(const Type* t) {
      IndexType idx = 0;
      auto it = dict.find(t);
      if (it == dict.end()) {
        idx = (IndexType)dict.size();
        dict[t] = (uint32_t)dict.size();
      }
      else
        idx = it->second;
      writePOD(idx);
    }

    void writeString(const char* txt) {
      uint32_t len = (uint32_t)strlen(txt) + 1;
      writePOD(len);
      writeBytes(txt, len);
    }

    void write(Ref r) {
      auto binary_io = r.type()->propByType<binaryIO>();

      writeType(r.type());
      if (binary_io)
        binary_io->write(*this, r);

      // Data members
      size_t s = size();
      uint8_t n = 0;
      writePOD(n);
      r.type()->data([&](const Data* d) {
        write(r.get(d));
        ++n;
        });

      // Update n members
      *(begin() + s) = n;
    }

    void close() {
      assert(!closed);
      Buffer b;
      b.writePOD(magic_header_types);
      b.writePOD((uint32_t)dict.size());
      for (auto it : dict) {
        b.writeString(it.first->name());
      }
      // Insert the header at the beginning of the buffer
      insert(begin(), b.begin(), b.end());
      closed = true;
    }

  };

  class BinParser {

    using IndexType = uint32_t;
    bool  header_parsed = false;
    
    uint32_t ntypes = 0;
    struct   SavedType {
      const char* name = nullptr;
      const Type* type = nullptr;
    };

    std::vector<SavedType> types;
    const uint8_t* top = nullptr;
    const uint8_t* end = nullptr;

  public:

    BinParser(const Buffer& buf) {
      assert(buf.data() && buf.size());
      top = buf.data();
      end = top + buf.size();
      parseHeader();
    }

    const uint8_t* consume(size_t nbytes) {
      const uint8_t* curr = top;
      top += nbytes;
      assert(top <= end);
      return curr;
    }

    void readBytes(void* addr, size_t nbytes) {
      memcpy(addr, consume(nbytes), nbytes);
    }

    template< typename POD>
    void readPOD(POD& pod) {
      readBytes(&pod, sizeof(POD));
    }

    const char* readString() {
      uint32_t len = 0;
      readPOD(len);
      const char* txt = (const char*)consume(len);
      return txt;
    }

    uint32_t parseHeader() {
      uint32_t magic_header_types_read = 0;
      uint32_t num_types = 0;
      readPOD(magic_header_types_read);
      readPOD(num_types);
      assert(magic_header_types == magic_header_types_read);
      types.reserve(num_types);
      for( uint32_t i=0; i<num_types; ++i ) {
        const char* name = readString();
        SavedType s;
        s.name = name;
        s.type = Register::findType(name);
        assert(s.type || fatal("Can't find type named %s\n", name));
        types.push_back(s);
      }
      return true;
    }

    void read(Ref r) {

      // Read type
      IndexType type_idx = 0;
      readPOD(type_idx);
      assert(type_idx < types.size());
      const Type* t = types[type_idx].type;
      assert(r.type() == t);

      // Read the obj
      auto binary_io = t->propByType<binaryIO>();
      if (binary_io)
        binary_io->read(*this, r);

      // Data members
      uint8_t n = 0;
      readPOD(n);
      r.type()->data([&](const Data* d) {
        read(r.get(d));
        --n;
        });
      assert(n == 0);
    }

    static constexpr uint32_t magic_header_types = 0x55114422;
    static constexpr uint32_t magic_struct_types = 0x55114422;
    using IndexType = uint32_t;


  };



  /*
  // -----------------------------------------------------------------------------------
  template< typename Container>
  jsonIO makeVectorIO() {
    jsonIO j;
    j.to_json = [](json& j, const Ref& r) {
      const Container& container = *r.as<Container>();
      j = json::array();
      size_t idx = 0;
      for (auto& item : container) {
        Ref child(&container[idx]);
        json jitem;
        toJson(jitem, child);
        j[idx] = std::move(jitem);
        ++idx;
      }
    };
    j.from_json = [](const json& j, const Ref& r) {
      Container& container = *(Container*)r.as<Container>();
      container.clear();
      container.resize(j.size());
      for (size_t i = 0; i < j.size(); ++i) {
        Ref child(&container[i]);
        fromJson(j[i], child);
      }
    };
    return j;
  }

  // -------------------- Helper to declare std::vector<Item> with the json serialzier
  // Example: reflectVector<int>("int");
  template< typename ItemType, typename UserType = std::vector<ItemType>, typename... Property>
  Factory<UserType>& reflectVector(const char* name, Property &&... property) {
    static std::string vname = "std::vector<" + std::string(name) + ">";
    return reflect<UserType>(vname.c_str(), makeVectorIO<UserType>(), std::forward<Property>(property)...);
  }

  void registerCommonTypes();
  */

  void registerBinaryIOCommonTypes();

}