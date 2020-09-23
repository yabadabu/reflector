#pragma once

#include "nlohmann/json.hpp"
using nlohmann::json;

namespace Reflector {

  // -----------------------------------------------------------------------------------
  // Json IO
  // This PropType allow to customize how any type is serialized to/from json
  struct jsonIO {
    std::function<void(json& j, const Ref& r)> to_json;
    std::function<void(const json& j, const Ref& r)> from_json;
  };
  
  void toJson(json& jout, const Ref& r);
  void fromJson(const json& jout, const Ref& r);

  // -----------------------------------------------------------------------------------
  template< typename Container>
  jsonIO makeVectorIO() {
    jsonIO j;
    j.to_json = [](json& j, const Ref& r) {
      const Container& container = *r.as<Container>();
      j = json::array();
      size_t idx = 0;
      for (auto& item : container) {
        typename Container::const_reference item = container[idx];
        Ref child(&item);
        json jitem;
        toJson(jitem, child);
        j.push_back(jitem);
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
  // Example: reflectVector<int>("int")..;
  template< typename ItemType, typename UserType = std::vector<ItemType>, typename... Property>
  Factory<UserType>& reflectVector(const char* name, Property &&... property) {
    static std::string vname = "std::vector<" + std::string(name) + ">";
    return reflect<UserType>(vname.c_str(), makeVectorIO<UserType>(), std::forward<Property>(property)...);
  }

  void registerCommonTypes();

}