#include <iostream>
#include "Ipv4Address.hpp"
#include "Ipv6Address.hpp"

void testIpv4Address()
{
  std::cout << "=== Test: Ipv4Address ===" << std::endl;

  Ipv4Address addr1("127.0.0.1:8080");
  Ipv4Address addr2("127.0.0.1:8080");
  Ipv4Address addr3("127.0.0.1:80");
  Ipv4Address addr4("192.168.1.1:8080");

  std::cout << "addr1: " << addr1 << std::endl;
  std::cout << "addr2: " << addr2 << std::endl;
  std::cout << "addr3: " << addr3 << std::endl;
  std::cout << "addr4: " << addr4 << std::endl;

  std::cout << "addr1 == addr2: " << (addr1 == addr2 ? "true" : "false")
            << std::endl;
  std::cout << "addr1 == addr3: " << (addr1 == addr3 ? "true" : "false")
            << std::endl;
  std::cout << "addr1 < addr4 : " << (addr1 < addr4 ? "true" : "false")
            << std::endl;
  std::cout << "addr4 < addr1 : " << (addr4 < addr1 ? "true" : "false")
            << std::endl;

  std::string invalid_ipv4[] = {"127.0.0.1",     // no port
                                "127.0.0.1:",    // missing port
                                "127.0.0.1:0",   // port = 0
                                "256.0.0.1:80",  // invalid octet
                                "127.0.0:80",    // missing octet
                                "abc.def.ghi.jkl:80"};

  const size_t count = sizeof(invalid_ipv4) / sizeof(invalid_ipv4[0]);
  for (size_t i = 0; i < count; ++i)
  {
    try
    {
      Ipv4Address invalid(invalid_ipv4[i]);
      std::cerr << "❌ Fehler: Ungültige Eingabe akzeptiert: "
                << invalid_ipv4[i] << std::endl;
    }
    catch (const std::exception& e)
    {
      std::cout << "✅ Ausnahme korrekt geworfen für '" << invalid_ipv4[i]
                << "': " << e.what() << std::endl;
    }
  }
}

void testIpv6Address()
{
  std::cout << "=== Test: Ipv6Address ===" << std::endl;

  Ipv6Address addr1("[2001:0db8::1]:8080");
  Ipv6Address addr2("[2001:0db8::1]:8080");
  Ipv6Address addr3("[2001:0db8::2]:8080");

  std::cout << "addr1: " << addr1 << std::endl;
  std::cout << "addr2: " << addr2 << std::endl;
  std::cout << "addr3: " << addr3 << std::endl;

  std::cout << "addr1 == addr2: " << (addr1 == addr2 ? "true" : "false")
            << std::endl;
  std::cout << "addr1 == addr3: " << (addr1 == addr3 ? "true" : "false")
            << std::endl;
  std::cout << "addr1 < addr3 : " << (addr1 < addr3 ? "true" : "false")
            << std::endl;
  std::cout << "addr3 < addr1 : " << (addr3 < addr1 ? "true" : "false")
            << std::endl;

  std::string invalid_ipv6[] = {
      "2001::1:8080",        // missing brackets
      "[2001::1]8080",       // missing :
      "[2001::1]:",          // missing port
      "[2001::1]:0",         // port = 0
      "[2001::1::abcd]:80",  // double ::
      "[2001:gggg::1]:80"    // invalid hex
  };

  const size_t count = sizeof(invalid_ipv6) / sizeof(invalid_ipv6[0]);
  for (size_t i = 0; i < count; ++i)
  {
    try
    {
      Ipv6Address fail(invalid_ipv6[i]);
      std::cerr << "❌ Fehler: Ungültige Eingabe akzeptiert: "
                << invalid_ipv6[i] << std::endl;
    }
    catch (const std::exception& e)
    {
      std::cout << "✅ Ausnahme korrekt geworfen für '" << invalid_ipv6[i]
                << "': " << e.what() << std::endl;
    }
  }
}

void testMixedComparisons()
{
  std::cout << "=== Test: Vergleich IPv4 <-> IPv6 ===" << std::endl;

  Ipv4Address ipv4("127.0.0.1:8080");
  Ipv6Address ipv6(
      "[::ffff:7f00:1]:8080");  // IPv4-mapped IPv6 (optional, aber realistisch)
  Ipv6Address standard6("[2001:db8::1]:8080");

  std::cout << "ipv4: " << ipv4 << std::endl;
  std::cout << "ipv6: " << ipv6 << std::endl;
  std::cout << "standard6: " << standard6 << std::endl;

  std::cout << "ipv4 == ipv6: " << (ipv4 == ipv6 ? "true" : "false")
            << std::endl;
  std::cout << "ipv4 < standard6: " << (ipv4 < standard6 ? "true" : "false")
            << std::endl;
  std::cout << "standard6 < ipv4: " << (standard6 < ipv4 ? "true" : "false")
            << std::endl;
}

int main()
{
  try
  {
    testIpv4Address();
    std::cout << std::endl;
    testIpv6Address();
    std::cout << std::endl;
    testMixedComparisons();
  }
  catch (const std::exception& e)
  {
    std::cerr << "❌ Fehler: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "\n✅ Alle Tests abgeschlossen.\n";
  return 0;
}
