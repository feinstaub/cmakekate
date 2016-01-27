/*
    Copyright (c) 2016 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#pragma once

template<typename... Args> struct SELECT {
  template<typename C, typename R>
  static constexpr auto OVERLOAD_OF(R(C::*pmf)(Args...)) -> decltype(pmf)
  {
    return pmf;
  }
};

#define CMB_SELECT_BOTH(F, A1, A2) F(A1, A2)
#define CMB_SELECT_FIRST(F, A1, A2) F(A1)
#define CMB_SELECT_SECOND(F, A1, A2) F(A2)

#define CMB_FOR_EACH_SOURCE_TABLE_IMPL(F, SELECT) \
  SELECT(F, object_sources_cxx, "C++") \
  SELECT(F, generated_object_sources_cxx, "C++ (Generated)") \
  SELECT(F, header_sources, "Header") \
  SELECT(F, generated_header_sources, "Header (Generated)") \
  SELECT(F, object_sources_c, "C") \
  SELECT(F, generated_object_sources_c, "C (Generated)") \
  SELECT(F, extra_sources, "Extra") \
  SELECT(F, excluded_sources, "Excluded")

#define CMB_FOR_EACH_SOURCE_TYPE(F) \
    CMB_FOR_EACH_SOURCE_TABLE_IMPL(F, CMB_SELECT_FIRST)

#define CMB_FOR_EACH_SOURCE_TABLE(F) \
    CMB_FOR_EACH_SOURCE_TABLE_IMPL(F, CMB_SELECT_BOTH)

#define CMB_FOR_EACH_BUILD_PROPERTY_TYPE(F) \
  F(include_directories_cxx) \
  F(include_directories_c) \
  F(include_directories) \
  F(compile_definitions_cxx) \
  F(compile_definitions_c) \
  F(compile_definitions)
