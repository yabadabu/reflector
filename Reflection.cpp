// Reflection.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// -----------------------------------------------------------------------------------
#include "reflector.h"
using namespace Reflector;

#include <windows.h>
#include "utils.h"

void dumpProps(const PropsContainer& props_container) {
  props_container.props([](const Ref& r) {
    json j;
    toJson(j, r);
    dbg("    Prop: %s %s\n", r.type()->name(), j.dump().c_str());
    });
}

void dumpType(const Type* t) {
  dbg("Type:%s\n", t->name());
  dumpProps(*t);
  t->data([&](const Data* d) {
    assert(d->parent() && d->parent() == t);
    dbg("  %s (%s)\n", d->name(), d->type()->name());
    dumpProps(*d);
    });
}

void dumpTypes() {
  Register::types([](const Type* t) {
    dumpType(t); 
    });
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
  dbg("Ref has type %s\n", ref.type()->name());
  ref.type()->data([](const Data* d) {
    dbg("  Has member %s (%s)\n", d->name(), d->type()->name());
    });
  City* c = ref.tryAs<City>();
  if (c) 
    dbg("It's not a city...\n");
  House* h = ref.tryAs<House>();
  if (h) 
    dbg("It's a house... %d %f\n", h->life, h->size);
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

  registerCommonTypes();
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
  const Data* d_life = r_house.type()->data("Life");
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
  dbg("City: %s\n", j.dump(2, ' ').c_str());

  City city2;
  fromJson(j, Ref(&city2));
  json j2;
  Ref(&city2).get("council").get("Size").set(4.2f);
  city2.size = City::eSize::Small;
  toJson(j2, Ref(&city2));
  dbg("City2; %s\n", j2.dump(2, ' ').c_str());
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
