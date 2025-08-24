# 💻 Webserv: A Custom HTTP/1.1 Server

## Overview
`webserv` is a custom HTTP/1.1 server developed as part of the 42 curriculum. The project's goal is to implement a fully functional web server from scratch in C++98, without using any external libraries. It focuses on gaining a deep understanding of network programming, sockets, HTTP protocols, and event-driven architectures.

This server is designed to handle multiple client connections simultaneously, serve static files, and support essential HTTP methods and functionalities as defined in its configuration.

## ✨ Features
- **HTTP/1.1 Compliance**: Supports a subset of the HTTP/1.1 (RFC 9112) protocol.
- **Multiple Virtual Servers**: Handles requests for different domains and ports from a single process, based on the server_name and listen directives in the configuration.
- **Non-Blocking I/O**: Uses an event-driven model (`epoll`) to efficiently manage a large number of concurrent connections.
- **HTTP Methods**: Implements `GET`, `POST`, and `DELETE` methods.
- **Static File Serving**: Serves static content from specified root directories.
- **Directory Listing**: Automatically generates an HTML index for directories when enabled.
- **Custom Error Pages**: Uses custom, user-defined error pages for various HTTP status codes (e.g., 404 Not Found, 403 Forbidden).
- **CGI (Common Gateway Interface)**: Supports basic CGI execution for dynamic content generation.
- **Configuration File**: The server's behavior is fully customizable via a `.conf` file, similar to Nginx.
- **Keep-Alive support**: Allows multiple requests to be sent over a single TCP connection, improving performance by reducing connection overhead.

## ⚙️ Installation & Usage
**Note: This project is built using `epoll` and is therefore specific to Linux systems.**

**Building the Project**

1. Clone the repository
   ```
   git clone https://github.com/bgebetsb/webserv.git
   cd webserv
   ```
 2. Compile the project using the provided `Makefile`:
    ```
    make
    ```
**Running the Server**

```
./webserv [webserv.conf]
```
**Configuration File Example**

The server's behavior is defined in a configuration file. Here is a minimal example showing some of the core directives:
```
cgi_path .py /usr/bin/python3;
cgi_path .php /usr/bin/php-cgi;

server {
    listen 8080;
    server_name localhost;

    error_page 404 /404.html;

    location / {
        http_methods GET POST;
        root /var/www/html;
        index index.php index.html;
        cgi .php .py;
    }

    location /redirect/ {
        http_methods GET;
        return 301 http://$host/subfolder$request_uri;
    }
}
```
