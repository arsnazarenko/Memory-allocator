BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
DEP_DIR = $(BUILD_DIR)/dep
SRC_DIR = src
INCLUDE_DIR = include
TARGET = $(BUILD_DIR)/alloc

CC = clang
CFLAGS += -std=c11 -Wall -Wextra -Werror -pedantic -g -I ./$(INCLUDE_DIR)

SOURCES = $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: print_info build_dir $(TARGET)

print_info:
	@echo target: $(TARGET) "\n"\
	.c files dir: $(SRC_DIR) "\n"\
	.h files dir: $(INCLUDE_DIR) "\n"\
	.o files dir: $(OBJ_DIR) "\n"\
	.d files dir: $(DEP_DIR) "\n"

build_dir:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DEP_DIR)

#clang-tidy
clang-tidy:
	clang-tidy $(SRC_DIR)/*.c $(INCLUDE_DIR)/*.h -checks=-*,clang-analyzer-* -- $(CFLAGS)

#link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

#pull in dependency info for *existing* .o files
-include $(OBJS:$(OBJ_DIR)/%.o=$(DEP_DIR)/%.d)

$(OBJ_DIR)/%.o: src/%.c
	$(CC) -c $(CFLAGS) src/$*.c -o $(OBJ_DIR)/$*.o
	$(CC) -MM $(CFLAGS) src/$*.c > $(DEP_DIR)/$*.d
	@mv -f $(DEP_DIR)/$*.d $(DEP_DIR)/$*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $(DEP_DIR)/$*.d.tmp > $(DEP_DIR)/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(DEP_DIR)/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $(DEP_DIR)/$*.d
	@rm -f $(DEP_DIR)/$*.d.tmp

#remove compilation products
clean:
	rm -rf $(BUILD_DIR) *.log

.PHONY:
	clean build_dir


