/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "avb_messages.h"

extern "C" {

#include <stdio.h>
#include <string.h>

}  // extern C

namespace avb {

uint32_t RollbackIndexRequest::GetSerializedSize() const {
  return sizeof(value_) + sizeof(slot_);
}

uint32_t RollbackIndexRequest::Serialize(uint8_t* payload,
                                         const uint8_t* end) const {
  if (payload + GetSerializedSize() > end)
    return 0;
  memcpy(payload, &value_, sizeof(value_));
  payload += sizeof(value_);
  memcpy(payload, &slot_, sizeof(slot_));
  payload += sizeof(slot_);
  return GetSerializedSize();
}

int RollbackIndexRequest::Deserialize(const uint8_t* payload,
                                      const uint8_t* end) {
  if (payload + GetSerializedSize() != end)
    return ERR_NOT_VALID;
  memcpy(&value_, payload, sizeof(value_));
  payload += sizeof(value_);
  memcpy(&slot_, payload, sizeof(slot_));
  payload += sizeof(slot_);
  return NO_ERROR;
}

uint32_t RollbackIndexResponse::GetSerializedSize() const {
  return sizeof(value_);
}

uint32_t RollbackIndexResponse::Serialize(uint8_t* payload,
                                          const uint8_t* end) const {
  if (payload + GetSerializedSize() > end)
    return 0;
  memcpy(payload, &value_, sizeof(value_));
  payload += sizeof(value_);
  return GetSerializedSize();
}

int RollbackIndexResponse::Deserialize(const uint8_t* payload,
                                       const uint8_t* end) {
  if (payload + GetSerializedSize() != end)
    return ERR_NOT_VALID;
  memcpy(&value_, payload, sizeof(value_));
  payload += sizeof(value_);
  return NO_ERROR;
}

uint32_t GetVersionResponse::GetSerializedSize() const {
  return sizeof(version_);
}

uint32_t GetVersionResponse::Serialize(uint8_t* payload,
                                       const uint8_t* end) const {
  if (payload + GetSerializedSize() > end)
    return 0;
  memcpy(payload, &version_, sizeof(version_));
  payload += sizeof(version_);
  return GetSerializedSize();
}

int GetVersionResponse::Deserialize(const uint8_t* payload,
                                    const uint8_t* end) {
  if (payload + GetSerializedSize() != end)
    return ERR_NOT_VALID;
  memcpy(&version_, payload, sizeof(version_));
  payload += sizeof(version_);
  return NO_ERROR;
}

};  // namespace avb
