PLUGIN_NAME=demo
OUT_PATH=./
PLUGIN=$(OUT_PATH)$(PLUGIN_NAME).a

PLUGIN_SOURCES?=$(wildcard *.cpp)

OBJS_PATH=./objs
OBJS=$(foreach f,$(PLUGIN_SOURCES),$(OBJS_PATH)/$(basename $(f)).o)

CFLAGS = 
CFLAGS += -Wall -g
CFLAGS += -I.
CFLAGS += -DPLATFORM_OSX -DPLATFORM_APPLE -DCOMPILING_PLUGIN
#CFLAGS += -O2

CCFLAGS = $(CFLAGS)
CCFLAGS += -std=c++17

#LNKFLAGS = -shared -fPIC -undefined dynamic_lookup
LNKFLAGS = 

all : $(OBJS_PATH) $(PLUGIN)
	@echo All done

clean : 
	rm -rf $(PLUGIN)
	rm -rf $(OBJS_PATH)

$(OBJS_PATH) :
	@echo Creating folders
	@mkdir -p $(OBJS_PATH) 
	@mkdir -p $(OUT_PATH)

$(OBJS_PATH)/%.o : %.cpp $(DEPS)
	@echo C++ $<
	@$(CC) -c $(CCFLAGS) -o $@ $<

$(OBJS_PATH)/%.o : %.c $(DEPS)
	@echo C $<
	@$(CC) -c $(CFLAGS) -o $@ $<

$(OBJS_PATH)/%.o : $(SRC_PATH)/%.c $(DEPS)
	@echo C $<
	@$(CC) -c $(CFLAGS) -o $@ $<

$(PLUGIN) : $(OBJS_PATH) $(OBJS) 
	@echo Linking $@
	@$(CC) $(LNKFLAGS) -o $@ $(OBJS) -lc++ -ldl

.phony : clean all
