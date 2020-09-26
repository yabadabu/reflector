#pragma once

#include "nlohmann/json.hpp"
using nlohmann::json;

namespace REFLECTOR_NAMESPACE {

  // -----------------------------------------------------------------------------------
  // Json IO
  // This PropType allow to customize how any type is serialized to/from json
  struct jsonIO {
    std::function<void(json& j, Ref r)> to_json;
    std::function<void(const json& j, Ref r)> from_json;

    // This works like tags to customize how to load from json
    // Assume there is just one member in the struct,
    struct SingleValue {};
  };

  // Specialized funcions for Ref, which do the actual work
  REFLECTOR_API void toJson(json& jout, Ref r);
  REFLECTOR_API void fromJson(const json& jout, Ref r);

  // Template in case the user send the object directly
  template< typename T>
  REFLECTOR_API void toJson(json& jout, T* r) {
    static_assert(!std::is_same_v<T, Ref>, "using toJson with an address to a Ref. You probably want to send a Ref object, or the address of an object.");
    toJson(jout, Ref(r));
  }

  template< typename T>
  REFLECTOR_API void fromJson(const json& jout, T* r) {
    fromJson(jout, Ref(r));
  }

  // -----------------------------------------------------------------------------------
  template< typename Container>
  jsonIO makeVectorIO() {
    jsonIO j;
    j.to_json = [](json& j, Ref r) {
      const Container& container = *r.as<Container>();
      j = json::array();
      size_t idx = 0;
      for (auto& item : container) {
        json jitem;
        toJson(jitem, &container[idx]);
        j[idx] = std::move(jitem);
        ++idx;
      }
    };
    j.from_json = [](const json& j, Ref r) {
      Container& container = *r.as<Container>();
      container.clear();
      container.resize(j.size());
      for (size_t i = 0; i < j.size(); ++i)
        fromJson(j[i], &container[i]);
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

  REFLECTOR_API void registerCommonTypes();

}