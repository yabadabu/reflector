#include "reflector.h"

namespace Reflector {

  namespace Register {

    namespace details {
      std::vector< Type* > all_user_types;
    }

    void addType(Type* t) {
      assert(t);
      details::all_user_types.push_back(t);
    }

    void delType(Type* t) {
      assert(t);
      auto it = std::find(details::all_user_types.begin(), details::all_user_types.end(), t);
      assert(it != details::all_user_types.end());
      details::all_user_types.erase(it);
    }

  }

  // ---------------------------------------------------------
  void toJson(json& jout, const Ref& r) {
    const Type* t = r.type();
    assert(t);
    const jsonIO* jio = t->propByType<jsonIO>();
    if (jio) {
      jio->to_json(jout, r);
    }
    else {
      // Default behaviour is to iterate over all props and return an object
      jout = json();
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

  void fromJson(const json& j, const Ref& r) {
    const Type* t = r.type();
    assert(t);
    const jsonIO* jio = t->propByType<jsonIO>();
    if (jio) {
      jio->from_json(j, r);
    }
    else {
      t->data([&](const Data* d) {
        const char* key = d->name();
        if (!j.count(key))
          return;
        const json& jv = j[key];
        fromJson(jv, r.get(d));
        });
    }
  }

  // ---------------------------------------------------------
  static void int_to_json(json& jout, const Ref& r) {
    jout = *r.as<int>();
  }
  static void int_from_json(const json& j, const Ref& r) {
    int ival = j.get<int>();
    r.set(ival);
  }
  
  static void float_to_json(json& jout, const Ref& r) {
    jout = *r.as<float>();
  }
  static void float_from_json(const json& j, const Ref& r) {
    float ival = j.get<float>();
    r.set(ival);
  }

  static void string_to_json(json& j, const Ref& r) {
    j = *r.as<std::string>();
  }
  static void string_from_json(const json& j, const Ref& r) {
    std::string ival = j.get<std::string>();
    r.set(ival);
  }

  // ---------------------------------------------------------
  void registerCommonTypes() {
    reflect<jsonIO>("jsonIO");
    reflect<float>("f32", jsonIO{ &float_to_json, &float_from_json });
    reflect<int>("int", jsonIO{ &int_to_json, &int_from_json });
    reflect<std::string>("std::string", jsonIO{ &string_to_json, &string_from_json });
  }

}
