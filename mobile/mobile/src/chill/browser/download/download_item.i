// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "content/public/browser/download_item.h"
#include "content/public/browser/web_contents.h"
%}

namespace content {

class DownloadItem {
public:

  enum DownloadState {
      IN_PROGRESS = 0,
      COMPLETE,
      CANCELLED,
      INTERRUPTED,
  };

  void AddObserver(opera::DownloadItemObserver* observer);
  void RemoveObserver(opera::DownloadItemObserver* observer);

  std::string GetMimeType();
  unsigned int GetId() const;

  void Pause();
  void Resume();
  void Cancel(bool user_cancel);
  void Remove();

  base::Time GetStartTime() const;

private:
  DownloadItem();
  ~DownloadItem();
};

%extend DownloadItem {
  jobject GetJavaWebContents() {
    if (self->GetWebContents())
      return self->GetWebContents()->GetJavaWebContents().Release();
    return nullptr;
  }

  void SetDisplayName(std::string name) {
    base::FilePath temp(name);
    $self->SetDisplayName(temp);
  }

  std::string GetDisplayName() {
    base::FilePath name = $self->GetFileNameToReportUser();
    return name.value();
  }

  long GetReceivedBytes() {
    return (long) $self->GetReceivedBytes();
  }

  long GetSize() {
    return (long) $self->GetTotalBytes();
  }

  bool WasLastInterruptReasonShutdownOrCrash() {
    content::DownloadInterruptReason lastReason = $self->GetLastReason();
    content::DownloadInterruptReason userShutdownReason = content::DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN;
    content::DownloadInterruptReason applicationCrashReason = content::DOWNLOAD_INTERRUPT_REASON_CRASH;
    return lastReason == userShutdownReason || lastReason == applicationCrashReason;
  }

  std::string GetLastInterruptReason() {
    return DownloadInterruptReasonToString($self->GetLastReason());
  }
}

}
