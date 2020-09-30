#pragma once

#include <vector>
#include <functional>

void dbg(const char* fmt, ...);

namespace Reflector {

  class Type;
  class BinEncoder;
  class BinDecoder;
  using Buffer = std::vector< uint8_t >;

  // -----------------------------------------------------------------------------------
  // Binary IO
  struct binaryIO {
    std::function<void(BinEncoder& b, Ref r)>       write;
    std::function<void(BinDecoder& b, Ref r)> read;
  };

  static constexpr uint32_t magic_header_types = 0x55114422;
  static constexpr uint32_t magic_struct_types = 0x55114422;
  using IndexType = uint32_t;

  // -----------------------------------------------------------------------------------
  class BinEncoder {

    std::unordered_map< const Type*, IndexType > dict;
    bool closed = false;

    Buffer& store;

  public:
    
    BinEncoder(Buffer& buf) : store(buf) { 
      rewind();
    }

    void rewind() {
      store.clear();
      closed = false;
    }

    void writeBytes(const void* addr, size_t nbytes) {
      //dbg("[%05ld] io.bin.write      %ld bytes\n", size(), nbytes);
      assert(!closed);
      store.insert(store.end(), (uint8_t*)addr, (uint8_t*)addr + nbytes);
    }

    template< typename T >
    void writePOD(const T& t) {
      //dbg("[%05ld] io.bin.writePOD   %s\n", size(), resolve<T>()->name());
      assert(strcmp(resolve<T>()->name(), "TypeNameUnknown"));
      const void* addr = &t;
      store.insert(store.end(), (uint8_t*)addr, (uint8_t*)addr + sizeof(T));
      //writeBytes(&t, sizeof(T));
    }

    void writeType(const Type* t) {
      //dbg("[%05ld] io.bin.---.Type %s\n", size(), t->name());
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
      size_t s = store.size();
      uint8_t n = 0;
      writePOD(n);
      r.type()->data([&](const Data* d) {
        write(r.get(d));
        ++n;
        });

      // Update n members
      *(store.begin() + s) = n;
    }

    void close() {
      //dbg("[%05ld] io.bin.Writing Header\n", size());
      assert(!closed);
      Buffer header_buf;
      BinEncoder b(header_buf);
      b.writePOD(magic_header_types);
      b.writePOD((uint32_t)dict.size());

      std::vector< const Type* > types_to_save;
      types_to_save.resize(dict.size());
      for (auto it : dict) 
        types_to_save[it.second] = it.first;
      for (auto it : types_to_save)
        b.writeString(it->name());

      // Insert the header at the beginning of the buffer
      store.insert(store.begin(), header_buf.begin(), header_buf.end());
      closed = true;
      //dbg("[%05ld] io.bin.Closed\n", size());
    }

  };

  class BinDecoder {

    using IndexType = uint32_t;
    bool  header_parsed = false;
    bool  is_valid = false;
    
    uint32_t ntypes = 0;
    struct   SavedType {
      const char* name = nullptr;
      const Type* type = nullptr;
    };

    std::vector<SavedType> types;
    const uint8_t* base = nullptr;
    const uint8_t* top = nullptr;
    const uint8_t* end = nullptr;

    size_t readOffset() const { return top - base; }

  public:

    BinDecoder(const Buffer& buf) {
      assert(buf.data() && buf.size());
      base = top = buf.data();
      end = top + buf.size();
      is_valid = parseHeader();
    }

    bool isValid() const { return is_valid; }

    const void* consume(size_t nbytes) {
      const void* curr = top;
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

    bool parseHeader() {
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
        assert(s.type || REFLECTOR_ERROR("Can't identify type named %s\n", name));
        if (!s.type)
          return false;
        types.push_back(s);
      }
      // offset will return from the data sector
      base = top;
      return true;
    }

    void read(Ref r) {

      // Read type
      IndexType type_idx = 0;
      readPOD(type_idx);
      assert(type_idx < types.size());
      const Type* t = types[type_idx].type;
      dbg("[%5d] Read type %s\n", readOffset() - sizeof(IndexType), t->name());
      assert(t);
      assert(r.type());
      assert(r.type() == t || REFLECTOR_ERROR("Expected reading Ref %s but found type %s at offset %ld\n", r.type()->name(), t->name(), readOffset() ));

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

  // -----------------------------------------------------------------------------------
  template< typename Container>
  binaryIO binaryVectorIO() {
    binaryIO io;
    io.write = [](BinEncoder& b, Ref r) {
      const Container& container = *r.as<Container>();

      size_t num_elems = container.size();
      b.writePOD(num_elems);

      size_t idx = 0;
      while (idx < container.size()) {
        b.write(&container[idx]);
        ++idx;
      }
    };
    io.read = [](BinDecoder& b, Ref r) {
      Container& container = *r.as<Container>();
      container.clear();

      size_t num_elems = 0;
      b.readPOD(num_elems);

      container.resize(num_elems);
      for (size_t i = 0; i < num_elems; ++i)
        b.read(&container[i]);
    };
    return io;
  }

  void toBinary(Buffer& buf, Ref r);
  void fromBinary(const Buffer& buf, Ref r);
  void registerBinaryIOCommonTypes();

}