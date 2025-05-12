#include "Response.hpp"

class StaticResponse : public Response
{
 public:
  StaticResponse(int client_fd, int response_code);
  ~StaticResponse();

  void sendResponse(void);

 private:
  StaticResponse(const StaticResponse& other);
  StaticResponse& operator=(const StaticResponse& other);
};
