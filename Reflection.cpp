// Reflection.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// -----------------------------------------------------------------------------------
#include <windows.h>

#include "nlohmann/json.hpp"
using nlohmann::json;

#include "reflector.h"
using namespace Reflector;

// -----------------------------------------------------------------------------------
void int_to_json(json& jout, const Ref& r) {
  jout = *r.as<int>();
}
void int_from_json(const json& j, const Ref& r) {
  int ival = j.get<int>();
  r.set(ival);
}
void float_to_json(json& jout, const Ref& r) {
  jout = *r.as<float>();
}
void float_from_json(const json& j, const Ref& r) {
  float ival = j.get<float>();
  r.set(ival);
}
void string_to_json(json& j, const Ref& r) {
  j = *r.as<std::string>();
}
void string_from_json(const json& j, const Ref& r) {
  std::string ival = j.get<std::string>();
  r.set(ival);
}

// -----------------------------------------------------------------------------------
#include "named_values.h"
jsonIO makeEnumIO(const INamedValues* named_values) {
  jsonIO j;
  j.to_json = [named_values](json& j, const Ref& r) {
    const int* iaddr = (const int*)r.rawAddr();
    assert(iaddr);
    j = named_values->nameOfInt(*iaddr);
  };
  j.from_json = [named_values](const json& j, const Ref& r) {
    const std::string& txt = j.get<std::string>();
    int ival = named_values->intValueOf(txt.c_str());
    int* iaddr = (int*)r.rawAddr();
    *iaddr = ival;
  };
  return j;
}

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
template< typename ItemType, typename UserType = std::vector<ItemType>, typename... Property>
Factory<UserType>& reflectVector(const char* name, Property &&... property) {
  static TStr64 vname("std::vector<%s>", name);
  return reflect<UserType>(vname, makeVectorIO<UserType>(), std::forward<Property>(property)...);
}

// -----------------------------------------------------------------------------------
struct House {
  int life = 1;
  float size = 2.0f;
  void render() {
    dbg("Rendering house %d,%f\n", life, size);
  }
};

typedef std::vector<int> IDs;

struct City {
  House council;
  enum class eSize {
    Big, Medium, Small
  };
  eSize size = eSize::Medium;
  std::string name;
  std::vector<House> houses;
  IDs ids;
  void render() {
    dbg("Rendering city %s\n", name.c_str());
  }
};

void dumpRef(Ref ref) {
  printf("Ref has type %s\n", ref.type()->name());
  ref.type()->data([](const data* d) {
    printf("  Has member %s (%s)\n", d->name(), d->type()->name());
    });
  City* c = ref.as<City>();
  if (c) {
    printf("It's not a city...\n");
  }
  House* h = ref.as<House>();
  if (h) {
    printf("It's a house... %d %f\n", h->life, h->size);
  }
}

struct IntRange {
  int vmin = 1;
  int vmax = 10;
  IntRange() = default;
  IntRange( int new_vmin, int new_vmax ) 
    : vmin( new_vmin )
    , vmax( new_vmax ) 
  {}
};

// -----------------------------------------------------------------------------------
void registerTypes() {

  reflect<jsonIO>("jsonIO");
  reflect<float>("f32", jsonIO{ &float_to_json, &float_from_json });
  reflect<int>("int", jsonIO{ &int_to_json, &int_from_json });
  reflect<std::string>("std::string", jsonIO{ &string_to_json, &string_from_json });
  reflectVector<House>("House");
  reflectVector<int>("int");

  {
    static NamedValues<City::eSize> values = {
     { City::eSize::Big, "Big" },
     { City::eSize::Medium, "Medium" },
     { City::eSize::Small, "Small" },
    };
    reflect<City::eSize>("City.eSize", makeEnumIO(&values));
  }

  reflect<IntRange>("IntRange")
    .data<&IntRange::vmin>("min")
    .data<&IntRange::vmax>("max")
    ;

  reflect<City>("City")
    .data<&City::council>("council", IntRange(5, 15))
    .data<&City::name>("name")
    .data<&City::houses>("houses")
    .data<&City::ids>("ids")
    .data<&City::size>("size")
    .func<&City::render>("Render")
    ;

  reflect<House>("House")
    .data<&House::life>("Life")
    .data<&House::size>("Size")
    .func<&House::render>("Render")
    ;
}

// -----------------------------------------------------------------------------------
void testTypes() {
  City city;
  city.houses.resize(2);
  city.houses[0] = House{ 10,20.f };
  city.houses[1] = House{ 11,21.f };
  city.ids.emplace_back(99);
  city.ids.emplace_back(102);
  
  House house;
  Ref r_house(&house);
  dumpRef(r_house);
  house.life = 10;
  const data* d_life = r_house.type()->data("Life");
  assert(d_life);
  Ref r_life = r_house.get(d_life);
  int prev_life = *r_life.as<int>();
  assert(prev_life == 10);
  //r_house.set(d_life, 4);
  r_life.set(4);
  assert(house.life == 4);
  int new_life = *r_life.as<int>();
  assert(new_life == 4);

  r_house.invoke("Render");
  Ref(&city).invoke("Render");

  assert(resolve<double>());

  dumpRef(r_house);

  city.council = house;
  city.name = "Barcelona";

  json j;
  toJson(j, Ref(&city));
  printf("City: %s\n", j.dump(2, ' ').c_str());

  City city2;
  fromJson(j, Ref(&city2));
  json j2;
  Ref(&city2).get("council").get("Size").set(4.2f);
  city2.size = City::eSize::Small;
  toJson(j2, Ref(&city2));
  printf("City2; %s\n", j2.dump(2, ' ').c_str());
}

void myDbgHandler(const char* txt) {
  printf("%s", txt);
  ::OutputDebugString(txt);
}

void myFatalHandler(const char* txt) {
  myDbgHandler(txt);
  if (MessageBox(nullptr, txt, "Error", MB_RETRYCANCEL) == IDCANCEL) {
    __debugbreak();
  }
}

int main()
{
  dbg_handler = &myDbgHandler;
  fatal_handler = &myFatalHandler;

  registerTypes();
  dumpTypes();
  testTypes();
}
