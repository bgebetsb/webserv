#pragma once

enum RequestStatus {
  READING_HEADERS,
  READING_BODY,
  SENDING_RESPONSE,
  COMPLETED
};
