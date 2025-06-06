CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -pedantic -g

NAME := webserv

UTILS := utils/Endianness.cpp utils/string.cpp utils/strtoint.cpp utils/time.cpp
LOGGER := Logger/Logger.cpp
CONFIGS:= Configs/Configs.cpp Configs/configUtils.cpp
REQUESTS:= 		requests/Request.cpp \
							requests/Startline.cpp \
							requests/Headers.cpp \
							requests/PathValidation/PathValidation.cpp \
							requests/PathValidation/PreventEscape.cpp \
							requests/Post.cpp \
							
RESPONSES:=		responses/FileResponse.cpp \
							responses/RedirectResponse.cpp \
							responses/Response.cpp \
							responses/StaticResponse.cpp \
							responses/DirectoryListing.cpp \
							responses/CgiResponse.cpp \

PARSING := parsing/Parsing.cpp parsing/Chunked.cpp
							
GLOBALS:=	main.cpp Webserv.cpp
EPOLL:= epoll/EpollFd.cpp epoll/Connection.cpp epoll/Ipv4Connection.cpp epoll/Ipv6Connection.cpp \
				epoll/Listener.cpp epoll/PipeFd.cpp
IP:= ip/IpAddress.cpp ip/Ipv4Address.cpp ip/Ipv6Address.cpp ip/IpComparison.cpp
SRC := $(UTILS) $(LOGGER) $(CONFIGS) $(REQUESTS) $(GLOBALS) $(EPOLL) $(IP) $(RESPONSES) $(PARSING)
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
	$(CXX) -MMD -MP -I$(SRCDIR) $(CXXFLAGS) -c $< -o $@

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
