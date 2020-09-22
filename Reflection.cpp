// Reflection.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <type_traits>
#include <cassert>
#include <functional>
#include <unordered_map>

#include "nlohmann/json.hpp"
using nlohmann::json;

namespace R {

  struct type;
  class Ref;
  class data;

  template< typename UserType >
  type* resolve();

  // ----------------------------------------
  // A bag of owned properties. 
  class has_props {

  public:
    std::vector< Ref* > m_props;

    // Fallback for last template
    void initProp() { }

    // Register one single property, optionally more after that
    template<typename Property, typename... Other>
    void initProp(Property&& property, Other &&... other);

    // Iterate over all props
    template< typename Fn>
    void props(Fn fn) const {
      for (auto p : m_props)
        fn(*p);
    }
    
    // Get the value of a property, null if the prop does not exists
    template< typename PropType >
    const PropType* propByType() const;
  };

  // ----------------------------------------
  // Member definition
  class data : public has_props {

    template<typename> friend struct Factory;
    friend class Ref;

    friend struct type;
    const char* m_name = "unknown_data_name";
    bool        m_registered = false;
    const type* m_type = nullptr;
    const type* m_parent = nullptr;
    std::function<void(void* owner, const void* new_value)> m_setter;
    std::function<void*(void* owner)> m_getter;

    data() = default;


  public:
    const char* name() const { return m_name; }
    const struct type* parent() const { return m_parent; }
    const struct type* type() const { return m_type; }
  };

  // ----------------------------------------
  // Type definition
  struct type : has_props {
    const char*          m_name = nullptr;
    bool                 m_registered = false;
    std::vector< data* > m_datas;
    const char* name() const { return m_name; }

    type(const char* new_name)
      : m_name( new_name )
    {
    }

    template< typename Fn >
    void data(Fn fn) const {
      for (auto d : m_datas)
        fn(d);
    }

    const class data* data(const char* data_name) const {
      for (auto d : m_datas)
        if( strcmp( d->name(), data_name) == 0 )
          return d;
      return nullptr;
    }

    bool isRegistered() const { return m_registered; }
  };

  // --------------------------------------------
  // The registry...
  std::vector< type* > all_user_types;

  // --------------------------------------------
  template< typename UserType >
  type* resolve() {
    static type user_type("TypeNameUnknown");
    return &user_type;
  };

  // --------------------------------------------
  template< typename MainType >
  struct Factory {
    type* the_type = nullptr;

    template< auto Member, typename... Property>
    Factory<MainType>& data(const char* name, Property &&... property) noexcept {

      assert(the_type);

      // Member is:    int Factory::*
      // decltype(MainType().*Member) returns int&&
      typedef typename std::remove_reference_t<decltype(MainType().*Member)> Type;

      static ::data user_data; 

      if (!user_data.m_registered) {
        user_data.m_name = name;
        user_data.m_type = ::resolve<Type>();
        user_data.m_parent = the_type;
        user_data.m_registered = true;
        user_data.m_setter = [](void* owner, const void* new_value) {
          MainType* typed_owner = reinterpret_cast<MainType*>(owner);
          const Type* typed_new_value = reinterpret_cast<const Type*>(new_value);
          (*typed_owner).*Member = *typed_new_value;
        };
        user_data.m_getter = [](void* owner) -> void* {
          MainType* typed_owner = reinterpret_cast<MainType*>(owner);
          return &(typed_owner->*Member);
        };
        user_data.initProp(std::forward<Property>(property)...);
        the_type->m_datas.push_back(&user_data);
      }

      return *this;
    }
  };

  template< typename UserType, typename... Property>
  Factory<UserType>& reflect(const char* name, Property &&... property) noexcept {
    type* user_type = resolve<UserType>();

    assert(!user_type->isRegistered());

    user_type->m_name = name;
    user_type->m_registered = true;
    user_type->initProp(std::forward<Property>(property)...);
    all_user_types.push_back(user_type);

    static Factory<UserType> f{ user_type };
    return f;
  }

  // -------------------------------------------
  class Ref {
  public:
    const type* m_type = nullptr;
    void* m_addr = nullptr;

  public:
    
    inline bool isValid() const { return m_type != nullptr && m_addr != nullptr; }
    
    Ref() = default;

    Ref(const Ref& other) = default;
    Ref(Ref&& other) = default;

    Ref(const Ref* other) = delete;

    template< typename Obj >
    Ref(Obj* obj) :
      m_type(::resolve<Obj>()),
      m_addr(obj)
    {
    }

    inline const type* type() const { return m_type; }

    template< typename Obj>
    const Obj* as() const {
      return ::resolve<Obj>() == m_type ? reinterpret_cast<const Obj*>(m_addr) : nullptr;
    }

    template< typename Obj>
    Obj* as() {
      return ::resolve<Obj>() == m_type ? reinterpret_cast<Obj*>(m_addr) : nullptr;
    }

    template< typename Value >
    bool set(const Value& new_value) const {
      const Value* addr = as<Value>();
      assert(addr);
      // Not using the setter!! but we don't have the data accessor.
      *const_cast<Value*>(addr) = new_value;
      return true; 
    }

    template< typename Value >
    bool set(const data* d, const Value&& value) const {
      assert(d);
      assert(d->m_parent == m_type);
      assert(d->type() == ::resolve<Value>());
      d->m_setter(m_addr, &value);
      return true;
    }

    Ref get(const char* data_name) const {
      const data* d = type()->data(data_name);
      assert(d);
      return get(d);
    }

    Ref get(const data* d) const {
      assert(d);
      assert(d->parent() == m_type);
      Ref r;
      r.m_type = d->type();
      r.m_addr = d->m_getter(m_addr);
      return r;
    }
  };

  template<typename Property, typename... Other>
  void has_props::initProp(Property&& property, Other &&... other)
  {
    // We need to make a copy of the given property to take ownership
    Ref* r = new Ref;
    r->m_type = ::resolve<Property>();
    r->m_addr = new Property(std::move(property));
    m_props.push_back(r);
    printf("Adding property type %s\n", r->type()->name());
    initProp(std::forward<Other>(other)...);
  }

  template< typename PropType >
  const PropType* has_props::propByType() const {
    const struct type* t = ::resolve<PropType>();
    for (auto p : m_props) {
      if (p->type() == t) {
        return p->as<PropType>();
      }
    }
    return nullptr;
  }

  // -----------------------------------------------------------------------------------
  // Json IO
  // This PropType allow to customize how any type is serialized to/from json
  struct jsonIO {
    std::function<void(json& jout, const Ref& r)> to_json;
    std::function<void(const json& jout, const Ref& r)> from_json;
  };

  void toJson(json& jout, const Ref& r) {
    const type* t = r.type();
    assert(t);
    const jsonIO* jio = t->propByType<jsonIO>();
    if (jio) {
      jio->to_json(jout, r);
    }
    else {
      // Default behaviour is to iterate over all props and return an object
      jout = json();
      t->data([&](const data* d) {
        const char* key = d->name();
        json j;
        toJson(j, r.get(d));
        if (!jout.is_object())
          jout = json::object();
        jout[key] = std::move(j);
        });
    }
  }

  void fromJson(const json& j, const Ref& r) {
    const type* t = r.type();
    assert(t);
    const jsonIO* jio = t->propByType<jsonIO>();
    if (jio) {
      jio->from_json(j, r);
    }
    else {
      t->data([&](const data* d) {
        const char* key = d->name();
        if (!j.count(key))
          return;
        const json& jv = j[key];
        fromJson(jv, r.get(d));
        });
    }
  }

  void dumpProps(const has_props& props_container) {
    props_container.props([](const Ref& r) {
      json j;
      toJson(j, r);
      printf("    Prop: %s %s\n", r.type()->name(), j.dump().c_str());
      });
  }

  void dumpType(const struct type* t) {
    printf("Type:%s\n", t->name());
    dumpProps(*t);
    t->data([&](const class data* d) {
      assert(d->parent() && d->parent() == t);
      printf("  %s (%s)\n", d->name(), d->type()->name());
      dumpProps(*d);
      });
  }

  void dumpTypes() {
    for (auto t : all_user_types)
      dumpType(t);
  }

}

// -----------------------------------------------------------------------------------
using namespace R;

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
  
  reflect<IntRange>("IntRange")
    .data<&IntRange::vmin>("min")
    .data<&IntRange::vmax>("max")
    ;
  reflect<R::jsonIO>("jsonIO");

  reflect<float>("f32", jsonIO{ &float_to_json, &float_from_json });
  reflect<int>("int", jsonIO{ &int_to_json, &int_from_json });

  reflect<City::eSize>("City.eSize");
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
  Ref(&city2).get("council").get("Score").set(4.1f);
  toJson(j2, Ref(&city2));
  printf("City2; %s\n", j2.dump(2, ' ').c_str());


}

int main()
{
  registerTypes();
  dumpTypes();
  testTypes();
}

// members using fn
// confirm json of enums can work
// invoke fns?
// Release memory
