// Reflection.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// -----------------------------------------------------------------------------------
#include <windows.h>

#include "nlohmann/json.hpp"
using nlohmann::json;

#include "reflector.h"
using namespace Reflector;

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
  float score = 2.0f;
};

struct City {
  House council;
  enum class eSize {
    Big, Medium, Small
  };
  eSize size = eSize::Medium;
};

void dumpRef(Ref ref) {
  printf("Ref has type %s\n", ref.type()->name());
  ref.type()->data([&](const data* d) {
    printf("  Has member %s (%s)\n", d->name(), d->type()->name());
    });
  City* c = ref.as<City>();
  if (c) {
    printf("It's not a city...\n");
  }
  House* h = ref.as<House>();
  if (h) {
    printf("It's a house... %d %f\n", h->life, h->score);
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


void registerTypes() {

  reflect<jsonIO>("jsonIO");
  reflect<float>("f32", jsonIO{ &float_to_json, &float_from_json });
  reflect<int>("int", jsonIO{ &int_to_json, &int_from_json });

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
    .data<&City::council>("council", IntRange(5,15))
    .data<&City::size>("size");

  reflect<House>("House")
    .data<&House::life>("Life")
    .data<&House::score>("Score");
}


void testTypes() {
  City city;
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

  assert(resolve<double>());

  dumpRef(r_house);

  city.council = house;

  json j;
  toJson(j, Ref(&city));
  printf("City: %s\n", j.dump(2, ' ').c_str());

  City city2;
  fromJson(j, Ref(&city2));
  json j2;
  Ref(&city2).get("council").get("Score").set(4.2f);
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

// members using fn to set
// confirm json of enums can work
// base class, derived class
// Support for static arrays, dynamic arrays
// invoke fns?
// Release memory
