// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//

#ifndef CEF_LIBCEF_DLL_CTOCPP_BEFORE_DOWNLOAD_CALLBACK_CTOCPP_H_
#define CEF_LIBCEF_DLL_CTOCPP_BEFORE_DOWNLOAD_CALLBACK_CTOCPP_H_
#pragma once

#ifndef USING_CEF_SHARED
#pragma message("Warning: "__FILE__" may be accessed wrapper-side only")
#else  // USING_CEF_SHARED

#include "include/cef_download_handler.h"
#include "include/capi/cef_download_handler_capi.h"
#include "libcef_dll/ctocpp/ctocpp.h"

// Wrap a C structure with a C++ class.
// This class may be instantiated and accessed wrapper-side only.
class CefBeforeDownloadCallbackCToCpp
    : public CefCToCpp<CefBeforeDownloadCallbackCToCpp,
        CefBeforeDownloadCallback, cef_before_download_callback_t> {
 public:
  explicit CefBeforeDownloadCallbackCToCpp(cef_before_download_callback_t* str)
      : CefCToCpp<CefBeforeDownloadCallbackCToCpp, CefBeforeDownloadCallback,
          cef_before_download_callback_t>(str) {}

  // CefBeforeDownloadCallback methods
  virtual void Continue(const CefString& download_path,
      bool show_dialog) OVERRIDE;
};

#endif  // USING_CEF_SHARED
#endif  // CEF_LIBCEF_DLL_CTOCPP_BEFORE_DOWNLOAD_CALLBACK_CTOCPP_H_

