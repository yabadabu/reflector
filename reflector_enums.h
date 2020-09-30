#pragma once

// -----------------------------------------------------------------------------------
// Example to io to json/binary from an enum using the helper class INamedValues
#include "named_values.h"
jsonIO jsonEnumIO(const INamedValues* named_values) {
  jsonIO j;
  j.to_json = [named_values](json& j, Ref r) {
    const int* iaddr = (const int*)r.rawAddr();
    assert(iaddr);
    j = named_values->nameOfInt(*iaddr);
  };
  j.from_json = [named_values](const json& j, Ref r) {
    const std::string& txt = j.get<std::string>();
    int ival = named_values->intValueOf(txt.c_str());
    int* iaddr = (int*)r.rawAddr();
    *iaddr = ival;
  };
  return j;
}

binaryIO binaryEnumIO(const INamedValues* named_values) {
  (void)(named_values);
  binaryIO io;
  io.write = [](BinEncoder& b, Ref r) {
    const int* iaddr = (const int*)r.rawAddr();
    b.writePOD(*iaddr);
  };
  io.read = [](BinDecoder& b, Ref r) {
    int* iaddr = (int*)r.rawAddr();
    b.readPOD(*iaddr);
  };
  return io;
}
