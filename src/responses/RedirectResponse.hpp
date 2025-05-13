#include "Response.hpp"

class RedirectResponse : public Response
{
 public:
  RedirectResponse(int client_fd, const std::string& redirect_location);
  ~RedirectResponse();

 private:
  RedirectResponse(const RedirectResponse& other);
  RedirectResponse& operator=(const RedirectResponse& other);
};
