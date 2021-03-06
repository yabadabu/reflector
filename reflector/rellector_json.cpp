#include "reflector/reflector_json.h"

namespace REFLECTOR_NAMESPACE {

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
  void registerJsonIOCommonTypes() {
    reflect<jsonIO>("jsonIO");
    reflect<float>("float", jsonIO{ &basic_to_json<float>, &basic_from_json<float> });
    reflect<int>("int", jsonIO{ &basic_to_json<int>, &basic_from_json<int> });
    reflect<bool>("bool", jsonIO{ &basic_to_json<bool>, &basic_from_json<bool> });
    reflect<uint32_t>("u32", jsonIO{ &basic_to_json<uint32_t>, &basic_from_json<uint32_t> });
    reflect<uint8_t>("u8", jsonIO{ &basic_to_json<uint8_t>, &basic_from_json<uint8_t> });
    reflect<std::string>("std::string", jsonIO{ &basic_to_json<std::string>, &basic_from_json<std::string> });
  }

}
