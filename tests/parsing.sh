#!/bin/bash

if [[ "$WEBSERV_PORT" == "" || "$NGINX_PORT" == "" ]]; then
  echo "Environment variables missing, set \$WEBSERV_PORT and \$NGINX_PORT"
  exit 1
fi

testinput() {
  TOTAL_TESTS=$((TOTAL_TESTS + 1))
  if [ "$#" -ne "3" ]; then
    echo "Invalid number of arguments to testinput function"
    return 1
  fi

  if [[ "$CURRENT_PORT" == "" ]]; then
    echo "\$CURRENT_PORT not set"
    return 1
  fi

  echo -n "Test $3 => "

  output=$(echo -en "$1" | socat - TCP:127.0.0.1:"$CURRENT_PORT")
  responseline=$(echo "$output" | head -n 1)
  if [[ "$responseline" == "" ]]; then
    if [[ "$2" == "" ]]; then
      echo "OK"
      PASSED_TESTS=$((PASSED_TESTS + 1))
      return 0
    fi
    echo "Empty response, expected $2"
    FAILED_TESTS=$((FAILED_TESTS + 1))
    return 1
  else
    if [[ "$2" == "" ]]; then
      echo "Expected empty response, got $output"
      FAILED_TESTS=$((FAILED_TESTS + 1))
      return 1
    fi
  fi

  first=$(echo "$responseline" | awk '{print $1}')
  if [[ "$first" != "HTTP/1.1" ]]; then
    echo "Invalid first segment, expected HTTP/1.1"
    FAILED_TESTS=$((FAILED_TESTS + 1))
    return 1
  fi
  second=$(echo "$responseline" | awk '{print $2}')
  if [[ "$second" != "$2" ]]; then
    echo "Invalid response code, expected $2, got $responseline"
    FAILED_TESTS=$((FAILED_TESTS + 1))
    return 1
  fi
  PASSED_TESTS=$((PASSED_TESTS + 1))
  echo "OK"
}

launchTests() {
  TOTAL_TESTS=0
  PASSED_TESTS=0
  FAILED_TESTS=0
  echo "Testing $1"
  export CURRENT_PORT="$2"

  echo "============= Tests that should result in no response================="
  testinput "GET / HTTP/1.1" "" "First line not ending with \n"
  testinput "GET / HTTP/1.1\r\n" "" "only first line"
  testinput "GET / HTTP/1.1\r\nHost: whatever\r\n" "" "no empty line after headers"

  echo "============= Tests that should result in error responses================="
  testinput "GET / HTTP/1.\r\nHost: whatever\r\n\r\n" 400 "minor version missing"
  testinput "GET /  HTTP/1.1\r\nHost: whatever\r\n\r\n" 400 "two spaces after path"
  testinput "GET/ HTTP/1.1\r\nHost: whatever\r\n\r\n" 400 "no space before path"
  testinput "GET / HTTP/1a1\r\nHost: whatever\r\n\r\n" 400 "No dot between minor and major version"
  testinput "GET / HTTP/1.01\r\nHost: whatever\r\n\r\n" 400 "Multiple digits in HTTP version"
  testinput "GET / HTTP/1.2\r\nHost: whatever\r\n\r\n" 505 "Unsupported minor version"
  testinput "GET / HTTP/2.1\r\nHost: whatever\r\n\r\n" 505 "Unsupported major version"
  testinput "GET / HTTP/1.1\r\n\r\n" 400 "host header missing"
  testinput "GET / HTTP/1.1\r\nHost: whatever\r\nHost: oida\r\n\r\n" 400 "duplicate host header"
  testinput "GET / HTTP/1.1\nHost: whatever.com:a\n\n" 400 "non-numeric port in host header"
  testinput "GET / HTTP/1.1\r\nHost: whatever.com%\r\n\r\n" 400 "Trailing percent in host header"
  testinput "GET / HTTP/1.1\r\nHost: whatever.com%fu\r\n\r\n" 400 "Non-hex characters in percent encoding"
  testinput "GET / HTTP/1.1\r\nHost: [::1\r\n\r\n" 400 "Unclosed square bracket ipv6"
  testinput "GET / HTTP/1.1\r\nHost: [qwer::1]\r\n\r\n" 400 "Invalid characters in ipv6"
  testinput "GET / HTTP/1.1\r\nHost: \"\r\n\r\n" 400 "Invalid characters in Host header"
  testinput "GET / HTTP/1.1\r\nHost: whatever\r\nHeader\r\n\r\n" 400 "Missing colon and value of header"
  testinput "GET / HTTP/1.1\r\nHost: whatever\r\nHeader:\r\n\r\n" 400 "Missing value of header"
  testinput "GET / HTTP/1.1\r\nHost: whatever\r\nHeader with spaces: value\r\n\r\n" 400 "Header name contains spaces"
  testinput "GET / HTTP/1.1\r\nHost: whatever\r\nHeader: \x01Oida\r\n\r\n" 400 "Invalid character in header"
  testinput "GET / HTTP/1.1\r\nHost: whatever\r\nContent-Length: ff\r\n\r\n" 400 "Content-Length without numeric value"
  testinput "GET / HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: asdfadslf\r\n\r\n" 501 "Invalid transfer-encoding value"
  testinput "POST /index.php HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: chunked\r\n\r\noasch\r\nHello\r\n0\r\n\r\n" 400 "Malformed chunk size"
  testinput "GET /index.php HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding:          something,chunked\r\n\r\n0\r\n\r\n" 501 "Invalid-transfer-encoding-value, without space"
  testinput "GET /index.php HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding:          chunked,something\r\n\r\n0\r\n\r\n" 400 "Chunked not the last encoding"
  testinput "GET /index.php HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: chunked; what=oida\r\n\r\n" 501 "Unrecognized transfer encoding parameters"
  testinput "GET /index.html HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: chunked,\r\n\r\n" 400 "Transfer-Encoding: No value after comma"
  testinput "GET /index.html HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: chunked;;\r\n\r\n" 400 "Transfer-Encoding: Two semicolons"
  testinput "GET /index.html HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: chunked;;oida=hello\r\n\r\n" 400 "Transfer-Encoding: Two semicolons"
  testinput "GET /index.html HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: chunked;oida==hello\r\n\r\n" 400 "Transfer-Encoding: Two equal signs"
  testinput "GET /index.html HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: chunked;oida=\"hello\r\n\r\n" 400 "Transfer-Encoding: Unclosed quote"
  testinput "GET /index.html HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: chunked, chunked\r\n\r\n" 400 "Transfer-Encoding: declared chunked twice"
  testinput "GET http://whatever.com%fu HTTP/1.1\r\nHost: whatever.com%fu\r\n\r\n" 400 "Non-hex characters in percent encoding (absolute form)"
  testinput "GET http://[::1 HTTP/1.1\r\nHost: [::1\r\n\r\n" 400 "Ipv6 host + unclosed square bracket (absolute form)"
  testinput "GET http://[qwer::1] HTTP/1.1\r\nHost: [qwer::1]\r\n\r\n" 400 "Invalid characters in ipv6 (absolute form)"
  testinput "GET http://whatever:a HTTP/1.1\r\nHost: whatever:a\r\n\r\n" 400 "Invalid port (absolute form)"
  testinput "POST /index.php HTTP/1.1\r\nHost: whatever\r\nTransfer-Encoding: Leberkas; Pepi=oida, chunked; extension=abc; otherextension = \"quoted string?\!\"\r\n\r\n5\r\nHello\r\n0\r\n\r\n" 501 "Crazy but a valid input, except unsupported encoding"

  echo "============= Tests that should result in 200 responses================="
  testinput "GET / HTTP/1.1\r\nHost: whatever\r\n\r\n" 200 "normal"
  testinput "GET / HTTP/1.1\r\nHost:whatever\r\n\r\n" 200 "no space after header name"
  testinput "GET / HTTP/1.1\r\nHost:          whatever       \r\n\r\n" 200 "multiple spaces in header"
  testinput "GET / HTTP/1.1\r\nHost:\twhatever\t\r\n\r\n" 200 "tabs in header"
  testinput "GET / HTTP/1.1\nHost: whatever\n\n" 200 "only \n"
  testinput "GET / HTTP/1.1\r\nHost: whatever.com:\r\n\r\n" 200 "Host header ending with colon"
  testinput "GET / HTTP/1.1\r\nHost: [::1]\r\n\r\n" 200 "Host is ipv6"
  testinput "GET http://[::1] HTTP/1.1\r\nHost: [::1]\r\n\r\n" 200 "Ipv6 host (absolute form)"

  echo "============================================================"
  echo "Total Tests: $TOTAL_TESTS, Passed: $PASSED_TESTS, Failed: $FAILED_TESTS"
}

launchTests "Webserv" $WEBSERV_PORT
echo -e "\n======================================\n"
# launchTests "nginx" $NGINX_PORT
