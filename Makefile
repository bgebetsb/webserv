CXX := c++
# TODO: Add back -Werror again
CXXFLAGS := -Wall -Wextra -std=c++98 -pedantic

NAME := webserv

UTILS := utils/Endianness.cpp

SRC := $(UTILS) Listener.cpp Server.cpp epoll/EpollEventData.cpp Webserv.cpp main.cpp

SRCDIR := src
OBJDIR := obj
OBJ := $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC))
DEPS := $(patsubst %.cpp, $(OBJDIR)/%.d, $(SRC))

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ)

-include $(DEPS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@mkdir -p $(@D)
	$(CXX) -MMD -MP $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	@if [ -d "$(OBJDIR)" ]; then \
		rm -rf "$(OBJDIR)"; \
		echo "Removing objects directory"; \
	else \
		echo "Nothing to do (objects directory doesn't exist)"; \
	fi

fclean: clean
	@if [ -f "$(NAME)" ]; then \
		echo "Removing executable"; \
		rm -f "$(NAME)"; \
	else \
		echo "Nothing to do (executable doesn't exist)"; \
	fi

compile_commands:
	bear -- $(MAKE) re

re: fclean all

.PHONY: all clean fclean re
