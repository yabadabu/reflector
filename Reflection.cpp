// Reflection.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// -----------------------------------------------------------------------------------
#include "reflector.h"
using namespace Reflector;

#include <windows.h>
#include "utils.h"

void dumpProps(const PropsContainer& props_container) {
  props_container.props([](Ref r) {
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

// -----------------------------------------------------------------------------------
struct House {
  int life = 1;
  float size = 2.0f;
  void render() {
    dbg("Rendering house %d,%f\n", life, size);
  }
  bool operator==(const House& h) const {
    return h.life == life && h.size == size;
  }
  ~House() {
    dbg("House destroyed %p..\n", this);
  }
  House() {
    dbg("House ctor-constructed %p..\n", this);
  }
  House(const House& other) noexcept {
    life = other.life;
    size = other.size;
    dbg("House copy-constructed %p from %p..\n", this, &other);
  }
  House(House&& other) noexcept {
    life = other.life;
    size = other.size;
    dbg("House move-constructed %p from %p..\n", this, &other);
  }
  void operator=(const House& other) noexcept {
    life = other.life;
    size = other.size;
    dbg("House op= %p from %p..\n", this, &other);
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
  IntRange(int new_vmin, int new_vmax)
    : vmin(new_vmin)
    , vmax(new_vmax)
  {}
};


// -----------------------------------------------------------------------------------
void registerTypes() {

  dbg("Sizeof(Ref) = %ld\n", sizeof(Ref));
  dbg("Sizeof(Type) = %ld\n", sizeof(Type));
  dbg("Sizeof(Data) = %ld\n", sizeof(Data));
  dbg("Sizeof(Func) = %ld\n", sizeof(Func));
  dbg("Sizeof(PropsContainer) = %ld\n", sizeof(PropsContainer));

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
    //.func<&City::render>("Render")
    ;

  reflect<House>("House")
    .data<&House::life>("Life")
    .data<&House::size>("Size")
    //.func<&House::render>("Render")
    ;

}

// -----------------------------------------------------------------------------------
void testTypes() {

  // Create a dummy object
  City city;
  city.houses.resize(2);
  city.houses[0].life = 10;
  city.houses[0].size = 20.0f;
  city.houses[1].life = 11;
  city.houses[1].size = 21.0f;
  city.ids.emplace_back(99);
  city.ids.emplace_back(102);

  // Create a separate object
  House house;
  Ref r_house(&house);
  dumpRef(r_house);
  house.life = 10;

  // Get a Data obj to access the Life data member from any House
  const Data* d_life = r_house.type()->data("Life");
  assert(d_life);

  // Given a Ref to a house, get access to the Life data member
  Ref r_life = r_house.get(d_life);

  // Refs return the address of data members
  int* prev_life = r_life.as<int>();
  assert(prev_life && *prev_life == 10);

  // Set the value using the House
  r_house.set(d_life, 4);
  assert(house.life == 4);

  // Set the value using the Data member access
  r_life.set(5);
  assert(house.life == 5);

  // All calls are ready/writing on the same value
  int new_life = *r_life.as<int>();
  assert(new_life == 5);

  // Call to method Render using a reference
  //r_house.invoke("Render");
  //Ref(&city).invoke("Render");

  assert(resolve<double>());

  dumpRef(r_house);

  city.council = house;
  city.name = "Barcelona";

  // Convert obj to json
  json j;
  toJson(j, &city);
  dbg("City: %s\n", j.dump(2, ' ').c_str());

  // Convert json to obj
  City city2;
  fromJson(j, &city2);

  // Chain Data members to change a var down in the hierarchy of city2
  json j2;
  Ref(&city2).get("council").get("Size").set(4.2f);
  city2.size = City::eSize::Small;
  toJson(j2, &city2);
  dbg("City2; %s\n", j2.dump(2, ' ').c_str());

  // Test copy operator
  House h1;
  h1.life = 100;
  h1.size = 101;
  House h2;
  h2.life = 1;
  h2.size = 2;
  Ref r1(&h1);
  Ref r2(&h2);
  r1.copyFrom(r2);
  assert(h1 == h2);

  // Or directly...
  House h3;
  Ref(&h3).copyFrom(&h1);
  assert(h1 == h3);

}

// -----------------------------------------------------------
struct Base {
  int         score = 10;
  std::string name = "unnamed";
};

struct Derived1 : public Base {
  int         speed = 20;
};

struct Derived2 : public Base {
  float       accuracy = 6.0f;
};

void testBase() {

  // Define a base type and 2 derived classes
  reflect<Base>("Base")
    .data<&Base::score>("Score")
    .data<&Base::name>("name")
    ;

  reflect<Derived1>("Derived1")
    .base<Base>()
    .data<&Derived1::speed>("Speed")
    ;

  reflect<Derived2>("Derived2")
    .base<Base>()
    .data<&Derived2::accuracy>("Accuracy")
    ;

  Base base;
  base.name = "base";
  base.score = 10;
  Derived1 derived1;
  derived1.name = "derived1";
  derived1.score = 100;
  Derived2 derived2;
  derived2.name = "derived2";
  derived2.score = 200;

  Ref rbase(&base);
  Ref rd1(&derived1);
  Ref rd2(&derived2);

  assert(rbase.type() != rd1.type());
  assert(rbase.type() == rd1.type()->parent());
  assert(rbase.type() == rd2.type()->parent());
  assert(rd2.type()->derivesFrom(rbase.type()));
  assert(rd1.type()->derivesFrom(rbase.type()));
  assert(!rd2.type()->derivesFrom(rd1.type()));
  assert(!rd1.type()->derivesFrom(rd2.type()));

  const Data* dbase_score = resolve<Base>()->data("Score");
  Ref rbase_score = rbase.get(dbase_score);
  assert(*rbase_score.as<int>() == 10);

  // Use a data from base class in a derived class
  Ref rd1_score = rd1.get(dbase_score);
  assert(*rd1_score.as<int>() == 100);

  // And change values in derived clasess
  rd1.set(dbase_score, 101);
  assert(*rd1_score.as<int>() == 101);
  assert(derived1.score == 101);

  // Derived classes can query data members of the base class
  const Data* dd1_score = rd1.type()->data("Score");
  assert(dd1_score == dbase_score);

  // Use a data from base class in the second derived class
  Ref rd2_score = rd2.get(dbase_score);
  assert(*rd2_score.as<int>() == 200);
  assert(derived2.score == 200);

  // Export to json exports the base members...
  json jd1;
  ::toJson(jd1, &derived1);
  dbg("%s\n", jd1.dump().c_str());
  assert(jd1["Score"] == 101);
  assert(jd1["name"] == "derived1");
  assert(jd1["Speed"] == 20);

  json jd2;
  ::toJson(jd2, &derived2);
  dbg("%s\n", jd2.dump().c_str());
  assert(jd2["Score"] == 200);
  assert(jd2["name"] == "derived2");
  assert(jd2["Accuracy"] == 6.0f);

  // Can use a ref object to convert to json to get the same results
  json jd2b;
  ::toJson(jd2b, rd2);
  assert(jd2 == jd2b);

  json jbase;
  ::toJson(jbase, &base);
  dbg("%s\n", jbase.dump().c_str());
  assert(jbase["Score"] == 10);
  assert(jbase["name"] == "base");

  // Back to objects from json
  Derived1 derived1b;
  fromJson(jd1, &derived1b);
  assert(derived1.name == derived1b.name);
  assert(derived1.score == derived1b.score);
  assert(derived1.speed == derived1b.speed);

  // Use the base json to read into a derived object. Some members are not updated 
  // as the json does not contain the data exclusive from derived
  Derived2 derived2b;
  fromJson(jbase, &derived2b);
  assert(base.name == derived2b.name);
  assert(base.score == derived2b.score);

  // Use the derived json to read into a base object
  Base b2;
  fromJson(jd2, &b2);
  assert(b2.name == derived2.name);
  assert(b2.score == derived2.score);

  // Check cast works
  Base* b3 = rd1.as<Base>();
  Base* b3b = rd1.tryAs<Base>();
  assert(b3 == &derived1);
  assert(b3 == b3b);

  // Copy from derived to base
  Base b4;
  bool rd1_to_b4 = Ref(&b4).copyFrom(rd1);
  assert(rd1_to_b4);
  assert(b4.name == derived1.name);
  assert(b4.score == derived1.score);

  // Copy from base to derived
  Derived1 derived1c;
  derived1c.speed = 31;
  bool b4_to_d1c = Ref(&derived1c).copyFrom(&b4);
  assert(b4_to_d1c);
  assert(b4.name == derived1c.name);
  assert(b4.score == derived1c.score);
  assert(derived1c.speed == 31);    // Speed field has not been modified
}

void dumpValue(const Value& v) {
  json j;
  dbg("v has type %s and data %p\n", v.type()->name(), v.ref().rawAddr());
  toJson(j, v.ref());
  dbg("  has value %s\n", j.dump().c_str());
}

void testValue() {
  dbg("Value tests begin...\n");
  dbg("sizeof(Value) = %ld\n", sizeof(Value));
  {
    Value v = 2;
    dumpValue(v);

    v.set(3.13f);
    dumpValue(v);

    {
      dbg("Creating a House object...\n");
      House h;
      v = h;
      dbg("About to destroy the House object...\n");
    }
    dumpValue(v);
    House* h2 = v.ref().as<House>();
    h2->life = 999;
    h2->size = 111;

    Value v2 = v;
    dumpValue(v2);
  }

  {
    Value v;
    std::vector<int> ids;
    ids.push_back(2);
    ids.emplace_back(21);
    v = ids;
    dumpValue(v);
  }
  dbg("Value tests end...\n");
}


// ------------------------------------------------------------------
template <typename Fn>
struct FunctionInfo;

template <typename Result, typename ...Args>
struct FunctionInfo<Result(Args...)> {
  using ResultType = std::decay_t<Result>;
  static constexpr size_t num_args = sizeof...(Args);
  static constexpr bool return_is_void = std::is_same_v<void, ResultType>;
};
//  FunctionInfo<decltype( __a_real_function__ )>::return_is_void);

// There is a family of functions, that recv as arg Result(*)(Args...) and result a 
// FunctionInfo associated to the same type
template<typename Result, typename ...Args>
constexpr FunctionInfo<Result(Args...)> asFunctionInfo(Result(*)(Args...));

// A pointer to a method of a class, also returns just a dummy fn with Result and args
template<typename Result, typename ...Args, typename UserType>
constexpr FunctionInfo<Result(Args...)> asFunctionInfo(Result(UserType::*)(Args...));

// Also, catch if the function is a const method
template<typename Result, typename ...Args, typename UserType>
constexpr FunctionInfo<Result(Args...)> asFunctionInfo(Result(UserType::*)(Args...) const);

// Everything else is void...
constexpr void asFunctionInfo(...);

// Then, the ugly expression: decltype(asFunctionInfo(std::declval< decltype(What) >()));
// Creates a fake value of the given type What
// Simulates a call to asFunctionInfo
// Just to retrieve the type of the result... FunctionInfo<Result(Args...)>
// if it's a function, or void otherwise

// -----------------------------------------------------------------------
struct Invoke {

  Value(*m_invoker)(size_t n, Value* values) = nullptr;

  template<auto What>
  struct Invokator {
    template<typename ...Args>
    inline Value invoke(Args... args) {
      using WhatInfo = decltype(asFunctionInfo(std::declval< decltype(What) >()));
      if constexpr (WhatInfo::return_is_void) {
        std::invoke(What, std::forward<Args>(args)...);
        return Value();
      }
      else {
        return std::invoke(What, std::forward<Args>(args)...);
      }
    }
  };

  /*

  template<auto What>
  void set() {
    m_invoker = [](size_t n, Value* values) -> Value {
      using WhatInfo = decltype(asFunctionInfo(std::declval< decltype(What) >()));
      dbg("Type of func info is %d\n", WhatInfo::return_is_void);

      Invokator<What> real_invokator;
      dbg("Invoker of %d vs %d\n", n, WhatInfo::num_args);

      if constexpr (WhatInfo::num_args == 0) {
        assert(n == 0);
        return real_invokator.invoke();
      }
      else if constexpr (WhatInfo::num_args == 1) {
        assert(n == 1);
        return real_invokator.invoke(values[0]);
      }
      else if constexpr (WhatInfo::num_args == 2) {
        assert(n == 2);
        return real_invokator.invoke(values[0], values[1]);
      }
      else if constexpr (WhatInfo::num_args == 3) {
        assert(n == 3);
        return real_invokator.invoke(values[0], values[1], values[2]);
      }
      else if constexpr (WhatInfo::num_args == 4) {
        assert(n == 4);
        return real_invokator.invoke(values[0], values[1], values[2], values[3]);
      }
      else {
        dbg("Is invocable with other combinations!\n");
        return 0;
      }
      return 0;
    };

  }
      */

  template<auto What, typename UserType>
  void set() {
    if (std::is_member_function_pointer_v< decltype(What) >) {
      m_invoker = [](size_t n, Value* values) -> Value {
        UserType* u = values[0].ref().as<UserType>();
        using WhatInfo = decltype(asFunctionInfo(std::declval< decltype(What) >()));

        dbg("Type of func info is %d\n", WhatInfo::return_is_void);
        dbg("User type %s\n", resolve<UserType>()->name());
        //dbg("is member %d\n", std::is_member_function_pointer_v< decltype(What) >);

        Invokator<What> invokator;

        dbg("Invoker of %d vs %d\n", n, WhatInfo::num_args);
        if constexpr (WhatInfo::num_args == 0) {
          return invokator.invoke(*u);
        }
        else if constexpr (WhatInfo::num_args == 1) {
          return invokator.invoke(*u, values[1]);
        }
        else if constexpr (WhatInfo::num_args == 2) {
          return invokator.invoke(*u, values[1], values[2]);
        }
        else if constexpr (WhatInfo::num_args == 3) {
          return invokator.invoke(*u, values[1], values[2], values[3]);
        }
        else if constexpr (WhatInfo::num_args == 4) {
          return invokator.invoke(*u, values[1], values[2], values[3], values[4]);
        }
        dbg("Is invocable with other combinations!\n");
        return 0;
      };
    }
    else {
      // raw method
    }
  }

  template<typename ...Args>
  Value invoke(Args... args) {
    // Flatten the args in an array, so we can use the common m_invoken signature
    Value vals[1 + sizeof...(Args)] = { std::forward<Args>(args)... };
    return m_invoker(sizeof...(Args), vals);
  }

};

void fn0() {
  dbg("At fn0. Returning nothing\n");
}

int fn1(int x, float f) {
  dbg("At fn1(%d,%f)\n", x, f);
  return x + 1;
}

float fn2(int x, float f1, float f2) {
  dbg("At fn2(%d,%f,%f)\n", x, f1, f2);
  return x + f1 + f2;
}

void fn2_void(int x, float f1, float f2) {
  dbg("At fn2(%d,%f,%f)\n", x, f1, f2);
}

struct TheObj {
  int id = 10;
  int sum(int a, int b) {
    return a + b + id;
  }
  int doble(int a) const {
    return a * 2 + id;
  }
};

void testFuncs() {
  /*
  Value vin = 2;
  Value vin_f = 2.1f;
  Value vout = fn1(vin, vin_f);
  dumpValue(vout);

  Invoke inv;
  inv.set<fn1>();
  int k = inv.invoke(vin, vin_f);
  dbg("Invoke returned %d\n", k);

  inv.set<fn2>();
  float f = inv.invoke(vin, vin_f, vin_f);
  dbg("Invoke(i,f,f) returned %f\n", f);

  bool r0 = std::is_same_v<void, std::invoke_result_t<decltype(fn0)>>;
  dbg("R0 is %d\n", r0);

  inv.set<fn0>();
  inv.invoke();
  dbg("Invoke()\n");

  inv.set<fn2_void>();
  inv.invoke(1, 3.14f, 100.0f);
  dbg("Invoke return void()\n");

  //using myFnType = FunctionInfo<void(int)>;
  using myFnType2 = FunctionInfo<decltype(fn2)>;
  dbg("Type2 of func info is %s\n", resolve<myFnType2>()->name());

  //int n = myFnType::num_args;
  //int n2 = myFnType2::num_args;
  //dbg("return is void:%ld\n", FunctionInfo<decltype(fn2)>::return_is_void);
  //dbg("return is void:%ld\n", FunctionInfo<decltype(fn2_void)>::return_is_void);
  */

  Invoke obj_inv;
  obj_inv.set<&TheObj::sum, TheObj>();
  TheObj the_obj;
  Value v = the_obj;
  int added = obj_inv.invoke(v, 10, 11);
  assert(added == 31);

  obj_inv.set<&TheObj::doble, TheObj>();
  int doble = obj_inv.invoke(v, 11);
  assert(doble == 32);

}

// -----------------------------------------------------------------
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
  testBase();
  dumpTypes();
  testValue();
  testFuncs();
}
