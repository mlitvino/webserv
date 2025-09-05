CC = c++
NAME = webserv

SRC_DIR = src
OBJ_DIR = objs
INC_DIR = incld

SRC_FILES =	main.cpp \
			te.cpp c.cpp

SRCS = $(foreach file,$(SRC_FILES),$(shell find $(SRC_DIR) -name "$(file)" -type f))
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
CPPFLAGS = -I$(INC_DIR) -MMD -MP -Wall -Wextra -Werror -std=c++20
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $@ -I$(INC_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

run: all
	./$(NAME)

debug: CPPFLAGS += -DDEBUG -g3
debug: all

.PHONY: all clean fclean re run

-include $(DEPS)
