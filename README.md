HoThis is my reduced toy version from the awesome https://github.com/skypjack/meta

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
	- Value: A container of any value, owning it's own memory
	- Func: Handles to methods of classes
- Copy values between refs
- Base class.
- Can reflect methods, by converting all args and return to Values
- Access members as path member1/1/member2

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
	void render(float scale) {
		printf( "Hi from render(%f)\n", scale );
	}
};

// Declare the type
reflect<House>("House")
  .data<&House::life>("Life")
  .data<&House::size>("Size")
  .func<&House::render>("MyRender");

// Access data members using names.
House my_house;
Ref r_house(&my_house);
Ref r_life = r_house.get("Life");
r_life.set(10);
assert( my_house.life == 10);

// Or chain them 
r_house.get("Life").set(11);

// You can also call methods
r_house.invoke("MyRender", 3.14f);

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
- [x] Can call methods given a Ref()
- [x] Confirm compiles without errors in OSX
- [x] I/O Binary. Not sure about the benefits. Sizes are not specially small compared to json
- [x] Reduced compressed size after using Varint encoding for integers (from 276 to 173 bytes)

## ToDo

- [ ] I/O ImGui
- [ ] Convert `const Data*` to DataPtr and `const Type*` to TypePtr o something similar
- [ ] More examples
- [ ] members using fn to get/set
- [ ] Support for dicts
- [ ] Release allocated memory. Value is not move aware, it's not calling the dtors, so
      for PODs it's fine, but anything else it's a problem
- [ ] Can instantiate a Value just from name? Need a template ctor
- [ ] Make name an abstracton/class?
