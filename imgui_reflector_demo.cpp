// Reflection.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// -----------------------------------------------------------------------------------
#include "imgui.h"
#include "reflector/reflector.h"
#include "reflector/reflector_json.h"
#include "reflector/reflector_binary.h"

// Because I want to use json & binary
#define enumIOs(lut)  jsonEnumIO(lut),                binaryEnumIO(lut)
#define vectorIOs(T)  jsonVectorIO<std::vector<T>>(), binaryVectorIO<std::vector<T>>()

using namespace REFLECTOR_NAMESPACE;
#include "utils.h"

#include "reflector_enums.h"

// -------------------- Helper to declare std::vector<Item> and adds the ios and assigns the name automatically
// Example: reflectVector<int>();
template< typename ItemType, typename UserType = std::vector<ItemType>, typename... Property>
Factory<UserType>& reflectVector(Property &&... property) {
  static std::string vname = "std::vector<" + std::string(resolve<ItemType>()->name()) + ">";
  return reflect<UserType>(vname.c_str(), vectorIOs(ItemType), std::forward<Property>(property)...);
}


namespace REFLECTOR_NAMESPACE {
  bool onImGui(Ref r, const Data* d = nullptr);

  static bool processDataMembers(Ref r) {
    bool changed = false;
    const Type* t = r.type();
    t->data([&](const Data* d) {

      //auto visible_fn = d->propByType<VisibleCondition>();
      //if (visible_fn && !visible_fn->condition(r))
      //  return;

      changed |= onImGui(r.get(d), d);
      });
    if (t->parent())
      changed |= processDataMembers(r.asBaseRef());
    return changed;
  }

  bool onImGui(Ref r, const Data* d) {
    bool changed = false;
    const Type* t = r.type();
    assert(t);

    //const RenderInMenuFn* custom_fn = t->propByType<RenderInMenuFn>();
    //if (custom_fn) {
    //  changed |= (*custom_fn)(r, d);
    //  return changed;
    //}

    //const imguiIO* fn = nut->propByType<imguiIO>();
    //if (fn && fn->imguiFn) {
    //  return fn->imguiFn(r, d);

    //}
    //else {
      if (d) {
        if (ImGui::TreeNode(d->name())) {

          changed |= processDataMembers(r);

          //if (r.type()->propByType<makeShowTransformHandle>()) {
          //  const TTransform* transform = r.tryAs<TTransform>();
          //  if (transform)
          //    changed |= ((TTransform*)transform)->debugInMenu();
          //}

          ImGui::TreePop();
        }
      }
      else {
        changed |= processDataMembers(r);
      }
//    }

    return changed;
  }


}


void fatal(const char* msg) {

}

struct House {
  int life = 1;
  float size = 2.0f;
  bool operator==(const House& h) const {
    return h.life == life && h.size == size;
  }
  House() = default;
  House(int new_life, float new_size) : life(new_life), size(new_size) {
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


void registerTypes() {

  registerBinaryIOCommonTypes();
  registerJsonIOCommonTypes();

  reflect<House>("House")
    .data<&House::life>("Life")
    .data<&House::size>("Size")
    ;

  {
    //static NamedValues<City::eSize> values = {
    // { City::eSize::Big, "Big" },
    // { City::eSize::Medium, "Medium" },
    // { City::eSize::Small, "Small" },
    //};
    //reflect<City::eSize>("City.eSize", enumIOs(&values));
  }

  reflect<City>("City")
    .data<&City::council>("council")
    .data<&City::name>("name")
    .data<&City::houses>("houses")
    .data<&City::ids>("ids")
    .data<&City::size>("size")
    .func<&City::render>("Render")
    ;

}

void showTypes() {
  Register::types([](const Type* t) {
    if (ImGui::TreeNode(t->name())) {
      t->data([](const Data* data) {
        ImGui::LabelText(data->type()->name(), "%s", data->name());
        });
      ImGui::TreePop();
    }
    });
}

void demoReflectorImgui() {

  static bool types_registered = false;
  if (!types_registered) {
    registerTypes();
    types_registered = true;
  }

  static House h1(5, 20.0f);
  ImGui::Begin("Reflector Demo");
  if (ImGui::TreeNode("Types")) {
    showTypes();
    ImGui::TreePop();
  }

  Data pdata;
  pdata.setName("House");
  onImGui(&h1, &pdata);

  ImGui::End();

}
