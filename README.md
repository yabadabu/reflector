This is my reduced toy version from the awesome https://github.com/skypjack/meta

# Features
- No macros
- read/write from/to json (wip for binary)
- Same serialization exposed to ImGui
- Types and Data members can be tagged with user-defined attributes
  - Editing ranges for ImGui
  - Custom visualization (Rad shown in degrees for example)
  - Conditional visualization
- Easy to plug support for enums. (Example included)
- Components
  - Type: Any C++ type
  - Data: Members associated to a type.
  - Ref: A pair of {Type + Address}. Address not owned by the Ref.
    You can create Ref from the address of a typed C++ object.
- Copy values between refs
- Base class.

# Requirements
- C++ 17
- Uses https://github.com/nlohmann/json for json input/output
- Requires to declare members at runtime. You can also unregister types.
- Memory is owned by the user types. If class `Foo` has a member data `const char* text`, we can export
  it to json, but we can't read back to a new instance of Foo automatically without knowing who creates/owns 
  the pointer.

# Some examples.
Check more in the sample code.

```cpp
struct House {
  int life = 1;
  float size = 2.0f;
};

// Declare the type
::reflect<House>("House")
  .data<&House::life>("Life")
  .data<&House::size>("Size");

// Access data members using names.
House my_house;
Ref r_house(&my_house);
Ref r_life = r_house.get("Life");
r_life.set(10);
assert( my_house.life == 10);

// Or chain them 
r_house.get("Life").set(11);

// Serialize to json
json j;
toJson(j, r_house);  // j = { "Life" : 10, "size" : 2.0 }

// Or send the addr of an object
toJson(j, &my_house);  // j = { "Life" : 10, "size" : 2.0 }

// Read from json
House your_house;
Ref r_house2;
fromJson(j, r_house2);
assert( your_house.life == my_house.life );
assert( your_house.size == my_house.size );

```

## Done

- [x] Improve register
- [x] Serialize to/from json using the nlohmann as json data type
- [x] invoke super basic fns
- [x] Support for std::strings
- [x] Support for std::vector
- [x] Check duped members, duped names
- [x] Confirm json of enums work
- [x] base class, derived class
- [x] Allow customization to export to dll's, namespace and error msgs

## ToDo

- [ ] More examples
- [ ] I/O ImGui
- [ ] I/O Binary
- [ ] members using fn to get/set
- [ ] Support for dicts
- [ ] Release allocated memory

