#pragma once

enum RequestStatus
{
  READING_START_LINE,
  READING_HEADERS,
  PROCESSING_REQUEST,
  READING_BODY,  // only used for POST requests
  SENDING_RESPONSE,
  SKIPPING_BODY,  // only used for GET or DELETE requests with body
  COMPLETED
};
