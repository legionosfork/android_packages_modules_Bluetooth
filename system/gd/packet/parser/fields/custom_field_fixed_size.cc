/*
 * Copyright 2019 The Android Open Source Project
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

#include "fields/custom_field_fixed_size.h"

#include "util.h"

const std::string CustomFieldFixedSize::kFieldType = "CustomField";

CustomFieldFixedSize::CustomFieldFixedSize(std::string name, std::string type_name, int size, ParseLocation loc)
    : ScalarField(name, size, loc), type_name_(type_name) {}

const std::string& CustomFieldFixedSize::GetFieldType() const {
  return CustomFieldFixedSize::kFieldType;
}

std::string CustomFieldFixedSize::GetDataType() const {
  return type_name_;
}

std::string CustomFieldFixedSize::GetRustDataType() const {
  return type_name_;
}

int CustomFieldFixedSize::GenBounds(std::ostream& s, Size start_offset, Size end_offset, Size size) const {
  if (!start_offset.empty()) {
    // Default to start if available.
    s << "auto " << GetName() << "_it = to_bound + (" << start_offset << ") / 8;";
  } else if (!end_offset.empty()) {
    Size byte_offset = size + end_offset;
    s << "auto " << GetName() << "_it = to_bound (+ to_bound.NumBytesRemaining() - (" << byte_offset << ") / 8);";
  } else {
    ERROR(this) << "Ambiguous offset for field.";
  }
  return 0;  // num_leading_bits
}

void CustomFieldFixedSize::GenExtractor(std::ostream& s, int, bool) const {
  s << "*" << GetName() << "_ptr = " << GetName() << "_it.extract<" << GetDataType() << ">();";
}

bool CustomFieldFixedSize::HasParameterValidator() const {
  return false;
}

void CustomFieldFixedSize::GenParameterValidator(std::ostream&) const {
  // Do nothing.
}

void CustomFieldFixedSize::GenInserter(std::ostream& s) const {
  s << "insert(" << GetName() << "_, i);";
}

void CustomFieldFixedSize::GenValidator(std::ostream&) const {
  // Do nothing.
}

void CustomFieldFixedSize::GenStringRepresentation(std::ostream& s, std::string accessor) const {
  // We assume that custom fields will have a ToString() method
  s << accessor << ".ToString()";
}

std::string CustomFieldFixedSize::GetRustParseDataType() const {
  return "[u8; " + std::to_string(GetSize().bytes()) + "]";
}

void CustomFieldFixedSize::GenRustGetter(std::ostream& s, Size start_offset, Size end_offset, std::string) const {
  Size size = GetSize();
  int num_leading_bits = GetRustBitOffset(s, start_offset, end_offset, GetSize());
  if (num_leading_bits != 0) {
    ERROR(this) << "must be byte aligned";
  }
  if (size.bits() % 8 != 0) {
    ERROR(this) << "size must be in full bytes";
  }

  s << "let " << GetName() << " = bytes[" << start_offset.bytes() << "..";
  s << start_offset.bytes() + size.bytes() << "].try_into()";
  s << ".map_err(|_| Error::InvalidPacketError)?;";
}

void CustomFieldFixedSize::GenRustWriter(std::ostream& s, Size start_offset, Size end_offset) const {
  Size size = GetSize();
  int num_leading_bits = GetRustBitOffset(s, start_offset, end_offset, GetSize());
  if (num_leading_bits != 0) {
    ERROR(this) << "must be byte aligned";
  }
  if (size.bits() % 8 != 0) {
    ERROR(this) << "size must be in full bytes";
  }

  s << "let " << GetName() << ": " << GetRustParseDataType() << " = self." << GetName() << ".into();";
  s << "buffer[" << start_offset.bytes() << ".." << start_offset.bytes() + GetSize().bytes() << "].copy_from_slice(&"
    << GetName() << ");";
}
