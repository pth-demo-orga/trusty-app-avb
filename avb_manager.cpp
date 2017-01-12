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

#include "avb_manager.h"

extern "C" {

#include <err.h>
#include <string.h>

}  // extern C

#include "secure_storage_interface.h"

#define DEBUG 0

const unsigned int kRollbackSlotMax = 32;

static const uint32_t kAvbVersion = 1;
static const char* kAvbRollbackFilename = "avb.rollback";
static const unsigned int kStorageIdLengthMax = 64;

static const uint32_t kTypeMask = 0xF000;
static const unsigned int kTypeShift = 12;

// Helper function to get |filename| and |slot| and based on type flag
// in |raw_slot|. |filename| is assumed to be allocated with
// kStorageIdLengthMax bytes.
static int GetFilenameAndSlot(uint32_t raw_slot,
                              char filename[kStorageIdLengthMax],
                              uint32_t* slot) {
  // Upper 16 bits should not be set
  if (raw_slot & 0xFFFF0000) {
    TLOGE("Error: Slot value %u invalid\n", raw_slot);
    return ERR_NOT_VALID;
  }
  // Mask type flag from raw slot to get index
  *slot = raw_slot & ~kTypeMask;
  if (*slot >= kRollbackSlotMax) {
    TLOGE("Error: Slot value %u larger than supported %u\n",
          *slot,
          kRollbackSlotMax);
    return ERR_NOT_VALID;
  }
  // Choose correct file for rollback index type
  strcpy(filename, kAvbRollbackFilename);
  int index_type = (raw_slot & kTypeMask) >> kTypeShift;
  char postfix[4];
  snprintf(postfix, 4, ".%01X", index_type);
  strcat(filename, postfix);
  return NO_ERROR;
}

namespace avb {

void AvbManager::ReadRollbackIndex(const RollbackIndexRequest& request,
                                   RollbackIndexResponse* response) {
  uint32_t slot;
  char filename[kStorageIdLengthMax];
  if (GetFilenameAndSlot(request.get_slot(), filename, &slot) < 0) {
    response->set_error(AvbError::kInvalid);
    TLOGE("Error: Invalid slot value: %u\n", request.get_slot());
    return;
  }

  int rc = storage_->open(filename);
  if (rc < 0) {
    response->set_error(AvbError::kInternal);
    TLOGE("Error: failed to open file %s: %d\n", filename, rc);
    return;
  }
  uint64_t size;
  rc = storage_->get_file_size(&size);
  if (rc < 0) {
    response->set_error(AvbError::kInternal);
    TLOGE("Error: failed to get size of file %s: %d\n", filename, rc);
    return;
  }

  // If no valid rollback counter file is found, initialize to 0
  uint64_t rollback_counter;
  if (size < kRollbackSlotMax * sizeof(uint64_t)) {
    TLOGD("No valid rollback index file found. Initializing to 0\n");
    uint64_t write_buf[kRollbackSlotMax] = {0};
    rc = storage_->write(0, write_buf, sizeof(write_buf));
    rollback_counter = 0;
  } else {
    TLOGD("Rollback index file found\n");
    rc = storage_->read(sizeof(rollback_counter) * slot,
                        &rollback_counter,
                        sizeof(rollback_counter));
  }

  if (rc < 0) {
    response->set_error(AvbError::kInternal);
    TLOGE("Error: reading storage object: %d\n", rc);
    return;
  }

  if (static_cast<size_t>(rc) < sizeof(rollback_counter)) {
    response->set_error(AvbError::kInternal);
    TLOGE("Error: invalid object size: %d\n", rc);
    return;
  }

  response->set_value(rollback_counter);
}

void AvbManager::WriteRollbackIndex(const RollbackIndexRequest& request,
                                    RollbackIndexResponse* response) {
  uint32_t slot;
  char filename[kStorageIdLengthMax];
  if (GetFilenameAndSlot(request.get_slot(), filename, &slot) < 0) {
    response->set_error(AvbError::kInvalid);
    TLOGE("Error: Invalid slot value: %u\n", request.get_slot());
    return;
  }

  int rc = storage_->open(filename);
  if (rc < 0) {
    response->set_error(AvbError::kInternal);
    TLOGE("Error: failed to open file %s: %d\n", filename, rc);
    return;
  }
  uint64_t size;
  rc = storage_->get_file_size(&size);
  if (rc < 0) {
    response->set_error(AvbError::kInternal);
    TLOGE("Error: failed to get size of file %s: %d\n", filename, rc);
    return;
  }

  // If no valid rollback counter file is found, initialize to 0
  uint64_t request_value = request.get_value();
  if (static_cast<size_t>(size) < kRollbackSlotMax * sizeof(uint64_t)) {
    TLOGD("No valid rollback index file found. Initializing to 0\n");
    uint64_t write_buf[kRollbackSlotMax] = {0};
    write_buf[slot] = request_value;
    rc = storage_->write(0, write_buf, sizeof(write_buf));
  } else {
    uint64_t rollback_counter;
    TLOGD("Found a rollback index file\n");
    rc = storage_->read(sizeof(rollback_counter) * slot,
                        &rollback_counter,
                        sizeof(rollback_counter));

    // Write value to specified slot in file
    if (request_value < rollback_counter) {
      response->set_error(AvbError::kInvalid);
      TLOGE(
          "Error: Requested write [%lu] is less than existing counter value "
          "[%lu]\n",
          static_cast<long unsigned>(request_value),
          static_cast<long unsigned>(rollback_counter));
      response->set_value(rollback_counter);
      return;
    }
    rc = storage_->write(
        sizeof(request_value) * slot, &request_value, sizeof(request_value));
  }

  if (rc < 0) {
    response->set_error(AvbError::kInternal);
    TLOGE("Error: accessing storage object [%d]\n", rc);
    return;
  }

  if (static_cast<size_t>(rc) < sizeof(request_value)) {
    response->set_error(AvbError::kInternal);
    TLOGE("Error: invalid object size [%d]\n", rc);
    return;
  }

  response->set_value(request_value);
}

void AvbManager::GetVersion(const GetVersionRequest& request,
                            GetVersionResponse* response) {
  response->set_version(kAvbVersion);
}

};  // namespace avb