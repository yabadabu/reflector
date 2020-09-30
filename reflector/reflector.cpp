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

}
