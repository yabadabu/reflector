#pragma once

#include <cassert>
#include <vector>
#include <new>

#ifndef REFLECTOR_ERROR
#define REFLECTOR_ERROR fatal
#endif

#ifndef REFLECTOR_API
#define REFLECTOR_API
#endif

#ifndef REFLECTOR_NAMESPACE
#define REFLECTOR_NAMESPACE Reflector
#endif

extern int REFLECTOR_ERROR(const char* fmt, ...);

namespace REFLECTOR_NAMESPACE {

  class Type;
  class Data;
  class Ref;

  template< typename UserType >
  Type* resolve();

  // ----------------------------------------
  // A bag of owned properties. Stores an array
  // of Refs, which is a Type + value addr
  class PropsContainer {

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
  class Data : public PropsContainer {

    template<typename> friend struct Factory;
    friend class Ref;

    friend class Type;
    const char* m_name = "unknown_data_name";
    const Type* m_type = nullptr;
    const Type* m_parent = nullptr;
    bool        m_registered = false;
    void      (*m_setter)(void* owner, const void* new_value) = nullptr;
    void*     (*m_getter)(void* owner) = nullptr;

  public:

    inline const char* name()   const { return m_name; }
    inline const Type* parent() const { return m_parent; }
    inline const Type* type()   const { return m_type; }

    void setName(const char* new_name) { m_name = new_name; }
  };

  // ----------------------------------------
  class Func {

    template<typename> friend struct Factory;
    friend class Ref;

    const char* m_name = "unknown_func_name";
    const Type* m_parent = nullptr;
    bool        m_registered = false;

    // Current support is just dummy methods with no args/no return values
    void      (*m_invoker)(void* owner) = nullptr;

  public:

    inline const char* name()   const { return m_name; }
    inline const Type* parent() const { return m_parent; }
  };

  // ----------------------------------------
  // Type definition
  class Type : public PropsContainer {

    template<typename> friend struct Factory;
    template<typename UserType, typename... Property>
    friend Factory<UserType>& reflect(const char* name, Property &&... property) noexcept;
    friend class Ref;

    const char*          m_name = nullptr;
    const Type*          m_parent = nullptr;
    std::vector< Data* > m_datas;
    std::vector< Func* > m_funcs;
    void               (*m_copy)(void* dst, const void* src) = nullptr;
    bool                 m_registered = false;

  public:

    Type(const char* new_name)
      : m_name(new_name)
    { }

    inline const char* name() const { return m_name; }
    inline const Type* parent() const { return m_parent; }

    template< typename Fn >
    void data(Fn fn) const {
      for (auto d : m_datas)
        fn(d);
      // Not iterating on the parent data members... Maybe we should?
    }

    // Iterate over all props and my parent props
    template< typename Fn>
    void props(Fn fn) const {
      PropsContainer::props(fn);
      if( m_parent )
        m_parent->props(fn);
    }

    const Data* data(const char* data_name) const {
      for (auto d : m_datas)
        if (strcmp(d->name(), data_name) == 0)
          return d;
      // Try to find in the parent type
      if (m_parent)
        return m_parent->data(data_name);
      return nullptr;
    }

    const Func* func(const char* func_name) const {
      for (auto d : m_funcs)
        if (strcmp(d->name(), func_name) == 0)
          return d;
      // Try in the parent type
      if (m_parent)
        return m_parent->func(func_name);
      return nullptr;
    }

    bool derivesFrom(const Type* other) const {
      const Type* t = this;
      while (t) {
        if (t == other)
          return true;
        t = t->m_parent;
      }
      return false;
    }

  };

  // --------------------------------------------
  // The global registry...
  namespace Register {

    REFLECTOR_API void addType(Type* new_type);
    REFLECTOR_API void delType(Type* new_type);

    namespace details {
      using AllTypesContainer = std::vector< Type* >;
      REFLECTOR_API const AllTypesContainer& allTypes();
    }

    // Iterate over all defined types
    template<typename Fn>
    void types(Fn fn) {
      for (auto t : details::allTypes())
        fn(t);
    }
  };
  
  // --------------------------------------------
  // Create an internal namespace to hide some details
  namespace details {
    template< typename UserType >
    REFLECTOR_API Type* resolve() noexcept {
      static Type user_type("TypeNameUnknown");
      return &user_type;
    };
  }

  // --------------------------------------------
  // Forward the call to the ::details method, but first remove const/volatile,etc, etc
  // so 'const int' is the same type as 'int'
  template< typename UserType >
  Type* resolve() {
    return details::resolve< std::decay_t<UserType> >();
  };
 
  // --------------------------------------------
  template< typename MainType >
  struct Factory {
    Type* the_type = nullptr;

    template< auto Member, typename... Property>
    Factory<MainType>& data(const char* name, Property &&... property) noexcept {

      assert(the_type);

      // Member is:                        int Factory::*
      // decltype(MainType().*Member) is:  int&&
      typedef typename std::remove_reference_t<decltype(MainType().*Member)> Type;

      static Data user_data;

      assert(!user_data.m_registered || REFLECTOR_ERROR("data(%s) is already defined in type %s, with name %s\n", name, the_type->name(), user_data.name()));
      user_data.m_name = name;
      user_data.m_type = resolve<Type>();
      user_data.m_parent = the_type;
      user_data.m_registered = true;
      user_data.m_setter = [](void* owner, const void* new_value) {
        MainType* typed_owner = reinterpret_cast<MainType*>(owner);
        const Type* typed_new_value = reinterpret_cast<const Type*>(new_value);
        typed_owner->*Member = *typed_new_value;
      };
      user_data.m_getter = [](void* owner) -> void* {
        MainType* typed_owner = reinterpret_cast<MainType*>(owner);
        return &(typed_owner->*Member);
      };
      user_data.initProp(std::forward<Property>(property)...);

      assert(the_type->data(name) == nullptr || REFLECTOR_ERROR("data name(%s) is being defined twice in type %s\n", name, the_type->name()));

      the_type->m_datas.push_back(&user_data);
      return *this;
    }

    template< typename BaseType >
    Factory<MainType>& base() noexcept {
      assert(the_type);

      const Type* base_type = resolve<BaseType>();
      
      // Already set to the same type. skip and don't complain
      if (base_type == the_type->m_parent)
        return *this;

      assert(!the_type->m_parent
        || REFLECTOR_ERROR("Can't set base class for %s to %s. It's already defined to be %s.", the_type->name(), base_type->name(), the_type->m_parent->name()));

      the_type->m_parent = base_type;
      return *this;
    }

    template< auto Method>
    Factory<MainType>& func(const char* name) noexcept {

      static Func user_func;
      assert(!user_func.m_registered || REFLECTOR_ERROR("func(%s) is already defined in type %s, with name %s\n", name, the_type->name(), user_func.name()));
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

  // Global function entry to start defining new types
  template< typename UserType, typename... Property>
  Factory<UserType>& reflect(const char* name, Property &&... property) noexcept {
    Type* user_type = resolve<UserType>();

    if (!user_type->m_registered) {
      user_type->m_registered = true;
      user_type->m_name = name;
      user_type->m_copy = [](void* dst, const void* src) {
        assert(src && dst);
        const UserType* typed_src = reinterpret_cast<const UserType*>(src);
        UserType* typed_dst = reinterpret_cast<UserType*>(dst);
        *typed_dst = *typed_src;
      };

      Register::addType(user_type);
    }
    else {
      assert(strcmp(user_type->m_name, name) == 0);
    }
    user_type->initProp(std::forward<Property>(property)...);

    static Factory<UserType> f{ user_type };
    return f;
  }

  // -------------------------------------------
  template< typename T >
  void unregisterType() {
    Register::delType(resolve<T>());
  }

  // -------------------------------------------
  // A Ref is a pair of pointers: address + type_info
  // The addr is not owned by the Ref, we are just referencing it.
  // It should be cheap to copy/move refs
  class Ref {

    template<typename Property, typename... Other>
    friend void PropsContainer::initProp(Property&& property, Other &&... other);

    friend class Value;

    const Type* m_type = nullptr;
          void* m_addr = nullptr;

  public:

    inline bool isValid() const { return m_type != nullptr && m_addr != nullptr; }

    Ref() = default;

    template< typename UserType >
    Ref(UserType* obj) :
      m_type(resolve<UserType>()),
      m_addr((void*)obj) {
    }

    inline const Type* type() const { return m_type; }
    inline const void* rawAddr() const { return m_addr; }

    // Will return null in case the type is not valid
    template< typename UserType>
    UserType* tryAs() {
      return m_type->derivesFrom(resolve<UserType>()) ? reinterpret_cast<UserType*>(m_addr) : nullptr;
    }

    // Will assert
    template< typename UserType>
    inline UserType* as() {
      assert(m_type->derivesFrom(resolve<UserType>()) || REFLECTOR_ERROR("Failed runtime conversion from current type %s to requested type %s\n", m_type->name(), resolve<UserType>()->name()));
      return reinterpret_cast<UserType*>(m_addr);
    }

    template< typename UserType>
    inline const UserType* as() const {
      assert(m_type->derivesFrom(resolve<UserType>()) || REFLECTOR_ERROR("Failed runtime conversion from current type %s to requested type const %s\n", m_type->name(), resolve<UserType>()->name()));
      return reinterpret_cast<const UserType*>(m_addr);
    }

    template< typename Value >
    void set(const Value& new_value) {
      Value* addr = as<Value>();
      assert(addr || REFLECTOR_ERROR("Can't set new value, Ref points to null object\n"));
      *addr = new_value;
    }

    template< typename Value >
    void set(const Data* d, const Value& value) const {
      assert(d || REFLECTOR_ERROR("Can't set new value. Invalid Data access object\n"));
      assert(m_type->derivesFrom(d->parent()) || REFLECTOR_ERROR("Can't set new value, Data %s is not part of current type %s, it's from type %s\n", d->name(), type()->name(), d->parent()->name()));
      assert(m_addr || REFLECTOR_ERROR("Can't set new value. Ref points to null object\n"));
      d->m_setter(m_addr, &value);
    }

    // -----------------------------------------
    Ref get(const char* data_name) const {
      assert(data_name);
      const Data* d = type()->data(data_name);
      assert(d || REFLECTOR_ERROR("Can't find Data member named %s in Ref type %s\n", data_name, type()->name()));
      return get(d);
    }

    Ref get(const Data* d) const {
      assert(d);
      assert(m_type->derivesFrom(d->parent()));
      Ref r;
      r.m_type = d->type();
      r.m_addr = d->m_getter(m_addr);
      return r;
    }

    // -----------------------------------------
    bool copyFrom(Ref r2) {
      assert(isValid());
      assert(r2.isValid());
      const Type* t2 = r2.type();
      if (m_type == t2)
        t2->m_copy(m_addr, r2.rawAddr());
      else if (t2->derivesFrom(m_type))
        type()->m_copy(m_addr, r2.rawAddr());
      else if (m_type->derivesFrom(t2))
        t2->m_copy(m_addr, r2.rawAddr());
      else
        return false;
      return true;
    }

    // -----------------------------------------
    void invoke(const char* func_name) const {
      const Func* f = type()->func(func_name);
      assert(f || REFLECTOR_ERROR("Function %s is not defined for type %s\n", func_name, type()->name()));
      invoke(f);
    }

    void invoke(const Func* f) const {
      assert(f);
      assert(isValid());
      assert(m_type->derivesFrom( f->parent() ) || REFLECTOR_ERROR("Function obj %s is not part of type %s\n", f->name(), type()->name()));
      f->m_invoker(m_addr);
    }

    // -----------------------------------------
    Ref asBaseRef() const {
      Ref r;
      r.m_addr = m_addr;
      r.m_type = type()->parent();
      return r;
    }


  };

  template<typename Property, typename... Other>
  void PropsContainer::initProp(Property&& property, Other &&... other) {
    // We need to make a copy of the given property to take ownership
    Ref r;
    r.m_type = resolve<Property>();
    r.m_addr = new Property(std::move(property));
    m_props.push_back(r);
    initProp(std::forward<Other>(other)...);
  }

  template< typename PropType >
  const PropType* PropsContainer::propByType() const {
    const Type* t = resolve<PropType>();
    for (auto& p : m_props) {
      if (p.type() == t)
        return p.as<PropType>();
    }
    return nullptr;
  }

  // ------------------------------
  // Will not call dtos, ctor, copy_ctors, move_semantics
  // So, use it only for POD's
  class Value {
    const Type* m_type = nullptr;
    std::vector< uint8_t > m_data;

  public:
    const Type* type() const { return m_type; }
    bool isValid() const { return m_type && !m_data.empty(); }
    Value() = default;

    template<typename T>
    Value(const T& new_value) {
      set(new_value);
    }

    template<typename T>
    void set(const T& new_value) {
      m_type = resolve<T>();
      m_data.resize(sizeof(T));
      new (m_data.data()) T(new_value);
    }

    template< typename T>
    const T& get() {
      assert(m_type == resolve<T>());
      return *reinterpret_cast<T>(m_data.data());
    }

    Ref ref() const {
      Ref r;
      r.m_addr = (void*)m_data.data();
      r.m_type = m_type;
      return r;
    }
  };

}

#include "reflector_json.h"
