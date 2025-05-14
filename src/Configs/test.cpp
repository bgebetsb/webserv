
#include <iostream>
#include <sstream>
#include "../exceptions/Fatal.hpp"
#include "Configs.hpp"

void test_error_page()
{
  std::cout << "=== Test: error_page ===" << std::endl;

  Configuration config;

  std::string arguments[11] = {
      "error_page 404 /404.html",
      "error_page 500 /500.html",
      "error_page 403 /403.html",
      "error_page 404 /404.html",
      "error_page abc /abc.html",
      "error_page 500 408 /500.html",
      "error_page ",
      "error_page 500",
      "error_page 500 500",
      "error_page 500 500 /500.html",
      "error_page 600 /600.html",
  };
  serv_config conf;
  for (int i = 0; i < 11; ++i)
  {
    try
    {
      std::cout << "Testing: " << arguments[i] << std::endl;
      std::stringstream ss(arguments[i]);
      config.process_server_item(ss, conf);
    } catch (const Fatal& e)
    {
      std::cerr << e.what() << std::endl;
    }
  }
  for (MErrors::iterator it = conf.error_pages.begin();
       it != conf.error_pages.end(); ++it)
  {
    std::cout << "Error code: " << it->first << ", filename: " << it->second
              << std::endl;
  }
  std::cout << "=== TEST ENDE ===" << std::endl << std::endl;
}

void test_server_name()
{
  std::cout << "=== Test: server_name ===" << std::endl;

  Configuration config;

  std::string arguments[11] = {
      "server_name localhost",
      "server_name example.com",
      "server_name www.example.com www.example2.com",
      "server_name google.com www.gidf.com",
      "server_name localhost",
      "server_name a b c d e f g h i j k l m n o p q r s t u v w x y z",
      "server_name 12345678901234567890123456789012345678901234567890",
      "server_name abc.def.ghi.jkl",
      "server_name !1228376",
      "server_name [abc.def.ghi.jkl]",
  };
  serv_config conf;
  for (int i = 0; i < 11; ++i)
  {
    try
    {
      std::cout << "Testing: " << arguments[i] << std::endl;
      std::stringstream ss(arguments[i]);
      config.process_server_item(ss, conf);
    } catch (const Fatal& e)
    {
      std::cerr << e.what() << std::endl;
    }
  }
  for (ServerNames::iterator it = conf.server_names.begin();
       it != conf.server_names.end(); ++it)
  {
    std::cout << "Server name: " << *it << std::endl;
  }
  std::cout << "=== TEST ENDE ===" << std::endl << std::endl;
}

int main(int argc, char* argv[])

{
  (void)argc;
  try
  {
    Configuration config(argv[1]);
    config.printConfigurations();
  } catch (const Fatal& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  } catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  // test_error_page();
  // test_server_name();
  // test_ipv4Address(); // In ip directory are tests for ipv4
  // test_ipv6Address(); // In ip directory are tests for ipv6
  return 0;
}
