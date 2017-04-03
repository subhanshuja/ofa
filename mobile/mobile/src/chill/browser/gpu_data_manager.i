// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/gpu_data_manager_observer.h"
#include "gpu/config/gpu_info.h"
%}

%nodefaultctor GpuDataManager;
%feature("director", assumeoverride=1) GpuDataManagerObserver;

namespace content {

class GpuDataManagerObserver {
 public:
  // Called for any observers whenever there is a GPU info update.
  virtual void OnGpuInfoUpdate();

 private:
  virtual ~GpuDataManagerObserver();
};

class GpuDataManager {
 public:
  static GpuDataManager* GetInstance();

  void AddObserver(GpuDataManagerObserver* observer);
  void RemoveObserver(GpuDataManagerObserver* observer);
  void RequestCompleteGpuInfoIfNeeded();
  gpu::GPUInfo GetGPUInfo();

 private:
  ~GpuDataManager();
};


}  // namespace content
