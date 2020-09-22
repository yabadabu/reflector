#pragma once

#include <type_traits>
#include <cassert>
#include <functional>
#include <unordered_map>
#include "utils.h"

namespace Reflector {

  struct type;
  class Ref;
  class data;

  template< typename UserType >
  type* resolve();

  // ----------------------------------------
  // A bag of owned properties. 
  class has_props {

  protected:

    std::vector< Ref > m_props;

  public:

    // Fallback for last template
    void initProp() { }

    // Register one single property, optionally more after that
    template<typename Property, typename... Other>
    void initProp(Property&& property, Other &&... other);

    // Iterate over all props
    template< typename Fn>
    void props(Fn fn) const {
      for (auto& p : m_props)
        fn(p);
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
  class func {

    template<typename> friend struct Factory;
    friend class Ref;

    const char* m_name = "unknown_func_name";
    bool        m_registered = false;
    const type* m_parent = nullptr;
    std::function<void(void* owner)> m_invoker;
  public:

    const char* name() const { return m_name; }
    const struct type* parent() const { return m_parent; }
  };

  // ----------------------------------------
  // Type definition
  struct type : has_props {

    const char* m_name = nullptr;
    bool                 m_registered = false;
    std::vector< data* > m_datas;
    std::vector< func* > m_funcs;

    type(const char* new_name)
      : m_name(new_name)
    { }

  public:
    const char* name() const { return m_name; }

    template< typename Fn >
    void data(Fn fn) const {
      for (auto d : m_datas)
        fn(d);
    }

    const class data* data(const char* data_name) const {
      for (auto d : m_datas)
        if (strcmp(d->name(), data_name) == 0)
          return d;
      return nullptr;
    }

    const class func* func(const char* func_name) const {
      for (auto d : m_funcs)
        if (strcmp(d->name(), func_name) == 0)
          return d;
      return nullptr;
    }

    bool isRegistered() const { return m_registered; }
  };

  // --------------------------------------------
  // The registry...
  std::vector< type* > all_user_types;

  // Create an internal namespace 
  namespace internal {
    // --------------------------------------------
    template< typename UserType >
    type* resolve() {
      static type user_type("TypeNameUnknown");
      return &user_type;
    };
  }

  // --------------------------------------------
  // Forward the call to the ::internal method, but first remove const/volatile,etc, etc
  template< typename UserType >
  type* resolve() {
    return internal::resolve< std::decay_t<UserType> >();
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

      assert(!user_data.m_registered || fatal("data(%s) is already defined in type %s, with name %s\n", name, the_type->name(), user_data.name()));
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

      assert(the_type->data(name) == nullptr || fatal("data name(%s) is being defined twice in type %s\n", name, the_type->name()));

      the_type->m_datas.push_back(&user_data);
      return *this;
    }

    template< auto Method>
    Factory<MainType>& func(const char* name) noexcept {

      static ::func user_func;
      assert(!user_func.m_registered || fatal("func(%s) is already defined in type %s, with name %s\n", name, the_type->name(), user_func.name()));
      user_func.m_name = name;
      user_func.m_registered = true;
      user_func.m_parent = the_type;
      user_func.m_invoker = [](void* owner) {
        MainType& typed_owner = *reinterpret_cast<MainType*>(owner);
        std::invoke(Method, typed_owner);
      };

      the_type->m_funcs.push_back(&user_func);

      return *this;
    }

  };

  template< typename UserType, typename... Property>
  Factory<UserType>& reflect(const char* name, Property &&... property) noexcept {
    type* user_type = resolve<UserType>();

    assert(!user_type->isRegistered() || fatal( "Type %s is already defined\n", name));

    user_type->m_name = name;
    user_type->m_registered = true;
    user_type->initProp(std::forward<Property>(property)...);
    all_user_types.push_back(user_type);

    static Factory<UserType> f{ user_type };
    return f;
  }

  // -------------------------------------------
  // A Ref is a pair of address + type_info
  // The addr is not owned by the Ref, we are just referencing it.
  // It should be cheat to copy/move refs
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
      m_addr((void*)obj)
    {
    }

    inline const type* type() const { return m_type; }
    const void* rawAddr() const { return m_addr; }

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

    // -----------------------------------------
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

    // -----------------------------------------
    void invoke(const char* func_name) const {
      const func* f = type()->func(func_name);
      assert(f || fatal( "Function %s is not defined for type %s\n", func_name, type()->name()));
      invoke(f);
    }

    void invoke(const func* f) const {
      assert(f);
      assert(f->parent() == m_type);
      f->m_invoker(m_addr);
    }
  };


  template<typename Property, typename... Other>
  void has_props::initProp(Property&& property, Other &&... other)
  {
    // We need to make a copy of the given property to take ownership
    Ref r;
    r.m_type = ::resolve<Property>();
    r.m_addr = new Property(std::move(property));
    m_props.push_back(r);
    printf("Adding property type %s\n", r.type()->name());
    initProp(std::forward<Other>(other)...);
  }

  template< typename PropType >
  const PropType* has_props::propByType() const {
    const struct type* t = ::resolve<PropType>();
    for (auto& p : m_props) {
      if (p.type() == t) {
        return p.as<PropType>();
      }
    }
    return nullptr;
  }

  // -----------------------------------------------------------------------------------
  // Json IO
  // This PropType allow to customize how any type is serialized to/from json
  struct jsonIO {
    std::function<void(json& j, const Ref& r)> to_json;
    std::function<void(const json& j, const Ref& r)> from_json;
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
