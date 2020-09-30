#include "reflector.h"

namespace REFLECTOR_NAMESPACE {

  const Func* Type::func(const char* func_name) const {
    for (auto d : m_funcs)
      if (strcmp(d->name(), func_name) == 0)
        return d;
    // Try in the parent type
    if (m_parent)
      return m_parent->func(func_name);
    return nullptr;
  }
  
  Ref Value::ref() const {
    Ref r;
    r.m_addr = (void*)m_data.data();
    r.m_type = m_type;
    return r;
  }

  // ---------------------------------------------------------
  namespace Register {

    namespace details {
      std::vector< Type* > all_user_types;
      const AllTypesContainer& allTypes() {
        return all_user_types;
      }

      AllTypesContainer::const_iterator findTypeByName(const char* name_to_search) {
        return std::lower_bound(allTypes().begin(), allTypes().end(), name_to_search, [](const Type* t, const char* name_to_search) {
          return strcmp(t->name(), name_to_search) < 0;
          });
      }

    }

    void addType(Type* t) {
      assert(t);
      auto it = details::findTypeByName(t->name());
      details::all_user_types.insert(it, t);
    }

    void delType(Type* t) {
      assert(t);
      auto it = details::findTypeByName(t->name());
      if (it != details::all_user_types.end())
        details::all_user_types.erase(it);
    }

    const Type* findType(const char* type_name) {
      auto it = details::findTypeByName(type_name);
      if (it != details::allTypes().end())
        return *it;
      return nullptr;
    }

  }

  // ---------------------------------------------------------
  void toJson(json& jout, Ref r) {
    const Type* t = r.type();
    assert(t);

    // Export first the base class
    if (t->parent()) {
      Ref rp = r.asBaseRef();
      toJson(jout, rp);
    }

    const jsonIO* jio = t->propByType<jsonIO>();
    if (jio) {
      jio->to_json(jout, r);
    }
    else {
      // Default behaviour is to iterate over all props and return an object
      t->data([&](const Data* d) {
        const char* key = d->name();
        json j;
        toJson(j, r.get(d));
        if (!jout.is_object())
          jout = json::object();
        jout[key] = std::move(j);
        });
    }

  }

  void fromJson(const json& j, Ref r) {
    const Type* t = r.type();
    assert(t);

    // Read first the base class
    if (t->parent()) {
      Ref rp = r.asBaseRef();
      fromJson(j, rp);
    }

    const jsonIO* jio = t->propByType<jsonIO>();
    if (jio) {
      jio->from_json(j, r);
    }
    else {
      t->data([&](const Data* d) {
        const char* key = d->name();
        if (!j.count(key)) {
          // Key not found
          if (d->propByType<jsonIO::SingleValue>()) {
            fromJson(j, r.get(d));
            return;
          }
        }
        else {
          const json& jv = j[key];
          fromJson(jv, r.get(d));
        }
        });
    }
  }

  // ---------------------------------------------------------
  template< typename T>
  static void basic_to_json(json& jout, Ref r) {
    jout = *r.as<T>();
  }
  template< typename T>
  static void basic_from_json(const json& j, Ref r) {
    T ival = j.get<T>();
    r.set(ival);
  }

  // ---------------------------------------------------------
  void registerCommonTypes() {
    reflect<jsonIO>("jsonIO");
    reflect<float>("float", jsonIO{ &basic_to_json<float>, &basic_from_json<float> });
    reflect<int>("int", jsonIO{ &basic_to_json<int>, &basic_from_json<int> });
    reflect<bool>("bool", jsonIO{ &basic_to_json<bool>, &basic_from_json<bool> });
    reflect<uint32_t>("uint", jsonIO{ &basic_to_json<uint32_t>, &basic_from_json<uint32_t> });
    reflect<std::string>("std::string", jsonIO{ &basic_to_json<std::string>, &basic_from_json<std::string> });
  }

}
